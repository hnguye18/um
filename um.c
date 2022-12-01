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
#include "seq.h"
#include "bitpack.h"
#include "registers.h"
#include "memory.h"

#define WORD_SIZE 32
#define OP_WIDTH 4
#define R_WIDTH 3
#define RA_LSB 6
#define RB_LSB 3
#define RC_LSB 0
#define NUM_REGISTERS 8

/* Struct definition of a UM_T which 
   contains two structs: 
   - Register_T representing the registers
   - Memory_T representing segmented memory */
struct UM_T {
    Registers_T reg;
    Memory_T mem;
};

/* Struct definition of a Memory_T which 
   contains two sequences: 
   - one holding pointers to UArray_T's representing segments
   - one holding pointers to uint32_t's representing free segments */
struct Memory_T {
        Seq_T segments;
        Seq_T free;
};

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
        opcode = Bitpack_getu(word, OP_WIDTH, WORD_SIZE - OP_WIDTH);
        prog_counter++;

        /* Load value */
        if (opcode == 13) {
            uint32_t value_size = WORD_SIZE - OP_WIDTH - R_WIDTH;
            ra = Bitpack_getu(word, R_WIDTH, value_size);
            uint32_t value = Bitpack_getu(word, value_size, 0);
            load_value(um, ra, value);
            continue;
        } 

        ra = Bitpack_getu(word, R_WIDTH, RA_LSB);
        rb = Bitpack_getu(word, R_WIDTH, RB_LSB);
        rc = Bitpack_getu(word, R_WIDTH, RC_LSB);

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

        default: assert(true);
    }
}

/* Name: populate
 * Input: UM_T struct,
 *        uint32_t "index" representing offset of segment,
 *        uint32_t "word" representing word to put into
 *            segment zero at given offset
 * Output: N/A
 * Does: Populates segment zero at offset "index" with value "word"
 * Error: Asserts if the UM_T struct is NULL
 * Notes: called by driver to populate segment zero with all instructions
 */
void populate(UM_T um, uint32_t index, uint32_t word)
{
    assert(um != NULL);
    memory_put(um->mem, 0, index, word);
}

 /* Name: conditional_move
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: if value in rc is not 0, moves value of rb into ra
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void conditional_move(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    if (registers_get(um->reg, rc) != 0) {
        registers_put(um->reg, ra, registers_get(um->reg, rb));
    }
}

/* Name: segmented_load
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: register a gets value of segment(val in rb) at offset(val in rc)
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void segmented_load(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rb_val = registers_get(um->reg, rb);
    uint32_t rc_val = registers_get(um->reg, rc);

    registers_put(um->reg, ra, memory_get(um->mem, rb_val, rc_val));
}

 /* Name: segmented_store
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: stores val in rc in segment(val in ra) at offset(val in rb)
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void segmented_store(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t ra_val = registers_get(um->reg, ra);
    uint32_t rb_val = registers_get(um->reg, rb);

    memory_put(um->mem, ra_val, rb_val, registers_get(um->reg, rc));
}

/* Name: add
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: adds values in rb and rc, and stores (sum % 2 ^ 32) in ra
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void add(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rb_val = registers_get(um->reg, rb);
    uint32_t rc_val = registers_get(um->reg, rc);

    registers_put(um->reg, ra, (rb_val + rc_val));
}

/* Name: multiply
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: multiplies values in rb and rc, and stores (product % 2 ^ 32) in ra
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void multiply(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rb_val = registers_get(um->reg, rb);
    uint32_t rc_val = registers_get(um->reg, rc);

    registers_put(um->reg, ra, (rb_val * rc_val));       
}

/* Name: divide
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: computes (value in rb) / (value in rc), and stores quotient in ra
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void divide(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rb_val = registers_get(um->reg, rb);
    uint32_t rc_val = registers_get(um->reg, rc);
    assert(rc_val != 0);

    registers_put(um->reg, ra, (rb_val / rc_val));
}

/* Name: nand
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: performs bitwise and on val in rb and val in rc,
 *       and stores the not (~) of the result in ra
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void nand(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rb_val = registers_get(um->reg, rb);
    uint32_t rc_val = registers_get(um->reg, rc);

    registers_put(um->reg, ra, ~(rb_val & rc_val));
}

/* Name: halt
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: frees all memory associated with the um and exits program
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void halt(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);
    
    um_free(&um);
    exit(EXIT_SUCCESS);
}

/* Name: map_segment
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: maps a segment of length of val in rc
 *       and stores index number of segment in rb
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void map_segment(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rc_val = registers_get(um->reg, rc);

    uint32_t index = memory_map(um->mem, rc_val);
    registers_put(um->reg, rb, index);
}

/* Name: unmap_segment
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: unmaps segment at value in rc
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
void unmap_segment(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rc_val = registers_get(um->reg, rc);

    memory_unmap(um->mem, rc_val);
}

/* Name: output
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: outputs the value in rc
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 *        Asserts if value in rc is not valid (not between 0 to 255 inclusive)
 */
void output(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    uint32_t rc_val = registers_get(um->reg, rc);
    assert(rc_val < 256);

    putchar(rc_val);
}

/* Name: input
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: takes in input from stdin and loads val into rc
 *       If end of input is signalled,
 *            rc gets 32-bit word in which every bit is 1 (~0)
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 * Note: since we used fgetc, the inputted value can never be greater than 255
 */
void input(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc) 
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS && rb < NUM_REGISTERS && rc < NUM_REGISTERS);

    int character = fgetc(stdin);

    if (character == EOF) {
        registers_put(um->reg, rc, ~0);
    }

    registers_put(um->reg, rc, character);
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

/* Name: load_value
 * Input: a UM_T struct, a register number, and a value
 * Output: N/A
 * Does: loads the passed in value into register ra
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if register is invalid
 */
void load_value(UM_T um, uint32_t ra, uint32_t val)
{
    assert(um != NULL);
    assert(ra < NUM_REGISTERS);

    registers_put(um->reg, ra, val);
}
