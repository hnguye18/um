/*
 * Implementation of the full UM, which includes functions to
 * allocate/deallocate memory associated with the UM_T struct,
 * function to populate segment zero with an instruction,
 * functions to execute instructions in segment zero,
 * and functions to perform all UM instructions
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "um.h"
#include "uarray.h"

#define WORD_SIZE 32
#define OP_WIDTH 4
#define R_WIDTH 3
#define RA_LSB 6
#define RB_LSB 3
#define RC_LSB 0
#define NUM_REGISTERS 8

/* Name: um_new
 * Input: a uint32_t representing the length of segment zero
 * Output: A newly allocated UM_T struct
 * Does: Allocates memory for a UM_T
 *       Creates a new Registers_T member and Memory_T member
 * Error: Asserts if memory is not allocated
 */
UM_T um_new(uint32_t length)
{
    UM_T um_new = malloc(sizeof(*um_new));
    assert(um_new != NULL);

    um_new->reg = registers_new();
    um_new->mem = memory_new(length);

    return um_new;
}

/* Name: um_free
 * Input: A pointer to a UM_T struct 
 * Output: N/A 
 * Does: Frees all memory associated with the struct and its members
 * Error: Asserts if UM_T struct is NULL
 */
void um_free(UM_T *um)
{
    assert((*um) != NULL);

    registers_free(&(*um)->reg);
    memory_free(&(*um)->mem);
    free(*um);
}

/* Name: um_execute
 * Input: a UM_T struct
 * Output: N/A
 * Does: Executes all instructions in segment zero until
 *       there is no instruction left or until there is a halt instruction
 *       Calls instruction_call to execute all instructions except
 *       load program and load value
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if segment zero is NULL at any point
 */
void um_execute(UM_T um)
{
    assert(um != NULL);

    UArray_T seg_zero = (UArray_T)Seq_get(um->mem->segments, 0);
    assert(seg_zero != NULL);

    int seg_zero_len = UArray_length(seg_zero);
    int prog_counter = 0;
    uint32_t opcode, ra, rb, rc, word;

    /* Execute words in segment zero until there are none left */
    while (prog_counter < seg_zero_len) {
        word = *(uint32_t *)UArray_at(seg_zero, prog_counter);
        //opcode = Bitpack_getu(word, OP_WIDTH, WORD_SIZE - OP_WIDTH);
        // op width = 4, second param = 28
        /* get the opcode */
        uint64_t opcode_shift = ((uint64_t)word << 32) >> 60;
        opcode = opcode_shift;

        prog_counter++;

        uint64_t ra_shift = 0;
        /* Load value */
        if (opcode == 13) {
            //uint32_t value_size = WORD_SIZE - OP_WIDTH - R_WIDTH;
            //ra = Bitpack_getu(word, R_WIDTH, value_size);
            ra_shift = ((uint64_t)word << 36) >> 61;
            // printf("%" PRIu64 "\n", ra_shift);
            ra = (uint32_t)ra_shift;
            //printf("%" PRIu32 "\n", ra);

            //uint32_t value = Bitpack_getu(word, value_size, 0);
            uint64_t value_shift = ((uint64_t)word << 39) >> 39;
            uint32_t value = value_shift;
            load_value(um, ra, value);
            continue;
        } 

        //ra = Bitpack_getu(word, R_WIDTH, RA_LSB);
        ra_shift = ((uint64_t)word << 55) >> 61;
        ra = ra_shift;
        //rb = Bitpack_getu(word, R_WIDTH, RB_LSB);
        uint64_t rb_shift = ((uint64_t)word << 58) >> 61;
        rb = rb_shift;
        //rc = Bitpack_getu(word, R_WIDTH, RC_LSB);
        uint64_t rc_shift = ((uint64_t)word << 61) >> 61;
        rc = rc_shift;

        /* Load Program */
        if (opcode == 12) {
            /* Updates programs counter*/
            prog_counter = load_program(um, ra, rb, rc);

            seg_zero = (UArray_T)Seq_get(um->mem->segments, 0);
            assert(seg_zero != NULL);

            seg_zero_len = UArray_length(seg_zero);
        } else {
            instruction_call(um, opcode, ra, rb, rc);
        }
    }
}

/* Name: instruction_call
 * Input: UM_T struct of the um
 *        opcode of instruction to be executed
 *        unint32_t's representing registers A, B, C
 * Output: N/A
 * Does: Executes opcodes 0 to 11 (cmove to input)
 * Error: Asserts if opcode is invalid
 *        Asserts if any register number is valid
 *        Asserts if UM_T sruct is NULL
 * Notes: is called by um_execute
 */
void instruction_call(UM_T um, Um_opcode op, uint32_t ra, 
                      uint32_t rb, uint32_t rc)
{
    assert(op >= 0 && op < 14);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);
    assert(um != NULL);

    switch (op) {
        case CMOV: conditional_move(um, ra, rb, rc);  break;
        case SLOAD: segmented_load(um, ra, rb, rc);   break;
        case SSTORE: segmented_store(um, ra, rb, rc); break;
        case ADD: add(um, ra, rb, rc);                break;
        case MUL: multiply(um, ra, rb, rc);           break;
        case DIV: divide(um, ra, rb, rc);             break;
        case NAND: nand(um, ra, rb, rc);              break;
        case HALT: halt(um, ra, rb, rc);              break;
        case MAP: map_segment(um, ra, rb, rc);        break;
        case UNMAP: unmap_segment(um, ra, rb, rc);    break;
        case OUT: output(um, ra, rb, rc);             break;
        case IN: input(um, ra, rb, rc);               break;

        default: assert(1);
    }
}

/* Name: load_program
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: a uint32_t representing the index that program should start at
 * Does: copies segment[rb value] into segment zero and returns rc value
 * Error: Asserts UM_T struct is NULL
 *        Asserts if any register number is valid
 */
uint32_t load_program(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rb_val = registers_get(um->reg, rb);

    /* If rb value is 0, 0 is already loaded into segment 0 */
    if (rb_val == 0) {
        return registers_get(um->reg, rc);
    }
    
    /* Get the segment to copy */
    UArray_T to_copy = (UArray_T)Seq_get(um->mem->segments, rb_val);
    assert(to_copy != NULL);

    /* Creating a copy with the same specifications */
    int seg_len = UArray_length(to_copy);
    UArray_T copy = UArray_new(seg_len, UArray_size(to_copy));
    assert(copy != NULL);

    /* Deep copying */
    for (int i = 0; i < seg_len; i++){
        *(uint32_t *)UArray_at(copy, i) = 
        *(uint32_t *)UArray_at(to_copy, i);
    }

    /* Freeing segment 0 and inserting the copy */
    UArray_T seg_zero = (UArray_T)Seq_get(um->mem->segments, 0);
    UArray_free(&seg_zero);
    Seq_put(um->mem->segments, 0, copy);

    return registers_get(um->reg, rc);
}