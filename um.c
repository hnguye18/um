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
#include <sys/stat.h>

#define WORD_SIZE 32
#define OP_WIDTH 4
#define R_WIDTH 3
#define RA_LSB 6
#define RB_LSB 3
#define RC_LSB 0
#define NUM_REGISTERS 8
#define HINT 10
#define REGISTER_LEN 8
#define W_SIZE 32
#define CHAR_SIZE 8
#define CHAR_PER_WORD 4

UM_T um_new(uint32_t length);
void populate_seg_zero(UM_T um, FILE *fp, uint32_t size);
uint32_t construct_word(FILE *fp);
void um_execute(UM_T um);
void um_free(UM_T *um);

int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./um <Um file>\n");
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "r");
    assert(fp != NULL);

    struct stat file_info;
    stat(argv[1], &file_info);
    uint32_t size = file_info.st_size / CHAR_PER_WORD;

    UM_T um = um_new(size);
    assert(um != NULL);

    populate_seg_zero(um, fp, size);

    fclose(fp);
    um_execute(um);

    um_free(&um);

    return EXIT_SUCCESS;
}

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

/* Name: memory_new
 * Input: a uint32_t representing the length of segment zero
 * Output: A newly allocated Memory_T struct
 * Does: * Creates a Sequence of UArray_T's, sets all segments to NULL besides
 *         segment 0, and segment 0 is initialized to be "length" long
         * Creates a Sequence of uint32_t pointers and 
           sets them to be the index of the unmapped segments
 * Error: Asserts if memory is not allocated
 */
Memory_T memory_new(uint32_t length)
{
        Memory_T m_new = malloc(sizeof(*m_new));
        assert(m_new != NULL);

        /* Creating the segments */
        m_new->segments = Seq_new(HINT);
        assert(m_new->segments != NULL);

        /* Creating the sequence to keep track of free segments */
        m_new->free = Seq_new(HINT);
        assert(m_new->free != NULL);

        /* Sets all segments to NULL and populates free segment sequence */
        for (int seg_num = 0; seg_num < HINT; ++seg_num) {
                Seq_addlo(m_new->segments, NULL);

                uint32_t *temp = malloc(sizeof(uint32_t));
                assert(temp != NULL);

                *temp = seg_num;
                Seq_addhi(m_new->free, temp);
        }

        /* Creating segment zero with proper length*/
        memory_map(m_new, length);

        return m_new;
}

/* Name: memory_free
 * Input: A pointer to a Memory_T struct 
 * Output: N/A 
 * Does: Frees all memory associated with the struct
 * Error: Asserts if struct is NULL
 */
void memory_free(Memory_T *m)
{
        assert(*m != NULL);

        /* Freeing the UArray_T segments */
        int seg_len = Seq_length((*m)->segments);
        for (int seg_num = 0; seg_num < seg_len; ++seg_num) {
                UArray_T aux = (UArray_T)Seq_remhi((*m)->segments);
                
                /* If the segment is unmapped, there is nothing to free */
                if (aux == NULL) {
                        continue;
                } else {
                        UArray_free(&aux);
                }
        }

        /* Freeing the uint32_t pointers */
        int free_len = Seq_length((*m)->free);
        for (int seg_num = 0; seg_num < free_len; ++seg_num) {
            uint32_t *integer = (uint32_t *)Seq_remhi((*m)->free);
            free(integer);
        }

        /* Freeing everything else */
        Seq_free(&(*m)->segments);
        Seq_free(&(*m)->free);
        free(*m);
}

/* Name: memory_put
 * Input: A Memory_T struct, a segment number, an offset, and a value
 * Output: N/A
 * Does: Inserts value at the specificed segment and offset
 * Error: Asserts if struct is NULL
 *        Asserts if segment is not mapped
 *        Asserts if offset is not mapped
 */
void memory_put(Memory_T m, uint32_t seg, uint32_t off, uint32_t val)
{
        assert(m != NULL);
        assert(seg < (uint32_t)Seq_length(m->segments));

        UArray_T queried_segment = (UArray_T)Seq_get(m->segments, seg);
        assert(queried_segment != NULL);
        assert(off < (uint32_t)UArray_length(queried_segment));

        *(uint32_t *)UArray_at(queried_segment, off) = val;
}

/* Name: memory_get
 * Input: A Memory_T struct, a segment number, and an offset
 * Output: A uint32_t which represents the value at that segment and offset
 * Does: Gets the value at the specified segment number and offset and returns
 * Error: Asserts if struct is NULL
 *        Asserts if segment is not mapped
 *        Asserts if offset is not mapped
 */
uint32_t memory_get(Memory_T m, uint32_t seg, uint32_t off)
{
        assert(m != NULL);
        assert(seg < (uint32_t)Seq_length(m->segments));
        
        UArray_T queried_segment = (UArray_T)Seq_get(m->segments, seg);
        assert(queried_segment != NULL);
        assert(off < (uint32_t)UArray_length(queried_segment));

        return *(uint32_t *)UArray_at(queried_segment, off);
}

/* Name: memory_map
 * Input: A Memory_T struct, a segment number, and segment length
 * Output: the index of the mapped segment
 * Does: Creates a segment that is "length" long 
 *       with all of the segment's values being zero and 
 *       returns index of the mapped segment
 * Error: Asserts if struct is NULL
 *        Asserts if memory for segment is not allocated
 */
uint32_t memory_map(Memory_T m, uint32_t length)
{
        assert(m != NULL);

        UArray_T seg = UArray_new(length, sizeof(uint32_t));
        assert(seg != NULL);

        /* Setting values in new segment to 0 */
        for (uint32_t arr_index = 0; arr_index < length; ++arr_index) {
                *(uint32_t *)UArray_at(seg, arr_index) = 0;
        }

        /* Mapping a segment */
        uint32_t index = Seq_length(m->segments);
        if (Seq_length(m->free) == 0) {
            /* If there are no free segments, 
               put UArray_T at end of sequence */
            Seq_addhi(m->segments, seg);
        } else {
            /* If there is a free segment, 
               get the index and put the UArray_T at that index */
            uint32_t *free_seg_num = (uint32_t *)Seq_remlo(m->free);
            index = *free_seg_num;
            free(free_seg_num);
            Seq_put(m->segments, index, seg);
        }

        return index;
}

/* Name: memory_unmap
 * Input: A Memory_T struct and a segment number
 * Output: N/A
 * Does: Unmaps a specified segment at the "seg_num" index and frees memory
 *       of the associated segment as well as adds that index back into the
 *       free segment sequence
 * Error: Asserts if struct is NULL
 *        Asserts if unmap segment 0
 *        Asserts if segment is NULL
 */
void memory_unmap(Memory_T m, uint32_t seg_num)
{
        assert(m != NULL);
        assert(seg_num != 0);

        UArray_T unmap = Seq_get(m->segments, seg_num);
        assert(unmap != NULL);

        UArray_free(&unmap);

        uint32_t *free_seg = malloc(sizeof(uint32_t));
        assert(free_seg != NULL);

        *free_seg = seg_num;
        Seq_addhi(m->free, free_seg);

        Seq_put(m->segments, seg_num, NULL);
}

/* registers */
/* Struct definition of a Register_T which 
   contains an unboxed array of uint32_t's to store vals in registers */
struct Registers_T {
        UArray_T registers;
}; 

/* Name: registers_new
 * Input: N/A
 * Output: A registers_T struct with values set to zero
 * Does: Initializes a Registers_T struct with a UArray_T with 8 indices and
 *       values set to zero
 * Error: Asserts if memory is not allocated
 */
Registers_T registers_new()
{
        Registers_T r_new = malloc(sizeof(*r_new));
        assert(r_new != NULL);

        r_new->registers = UArray_new(REGISTER_LEN, sizeof(uint32_t));
        assert(r_new->registers != NULL);

        /* Sets register's values to 0 */
        for (int index = 0; index < REGISTER_LEN; ++index) {
                *(uint32_t *)UArray_at(r_new->registers, index) = 0;
        }

        return r_new;
}

/* Name: registers_free
 * Input: A pointer to a Registers_T struct
 * Output: N/A
 * Does: Frees memory associated with the struct
 * Error: Asserts if struct is NULL
 */
void registers_free(Registers_T *r)
{
        assert(*r != NULL);

        UArray_free(&(*r)->registers);
        free(*r);
}

/* Name: registers_put
 * Input: A registers_t struct, a register index, and a value
 * Output: N/A
 * Does: Inserts the value into the UArray in the Registers_T struct at index
 *       num_register
 * Error: Asserts if invalid register
          Asserts if struct is NULL
 */
void registers_put(Registers_T r, uint32_t num_register, uint32_t value)
{
        assert(r != NULL);
        assert(num_register < REGISTER_LEN);

        *(uint32_t *)UArray_at(r->registers, num_register) = value;
}

/* Name: registers_get
 * Input: a registers_t struct and a register index
 * Output: a uint32_t representing the value in the register
 * Does: Gets the value at the index num_register in the UArray in the struct
 *       and returns
 * Error: Asserts if invalid register
 *        Asserts if struct is NULL
 */
uint32_t registers_get(Registers_T r, uint32_t num_register)
{
        assert(r != NULL);
        assert(num_register < REGISTER_LEN);

        return *(uint32_t *)UArray_at(r->registers, num_register);
}

/* um_driver.c */

/* Name: populate_seg_zero
 * Input: A UM_t struct, a file pointer, and a size
 * Output: N/A
 * Does: Populates segment zero with words from the file
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if fp does not exist
 */
void populate_seg_zero(UM_T um, FILE *fp, uint32_t size)
{
    assert(um != NULL);
    assert(fp != NULL);

    for (uint32_t index = 0; index < size; ++index) {
        uint32_t word = construct_word(fp);

        populate(um, index, word);
    }
}

/* Name: construct_word
 * Input: a file pointer
 * Output: a uint32_t representing a word
 * Does: grabs 8 bits in big endian order and 
 *       creates a 32 bit word which is returned
 * Error: Asserts if file pointer is NULL
 */
uint32_t construct_word(FILE *fp)
{
    assert(fp != NULL);

    uint32_t c = 0, word = 0;
    int bytes = W_SIZE / CHAR_SIZE;

    /* Reads in a char and creates word in big endian order */
    for (int c_loop = 0; c_loop < bytes; c_loop++) {
        c = getc(fp);
        assert(!feof(fp));

        unsigned lsb = W_SIZE - (CHAR_SIZE * c_loop) - CHAR_SIZE;
        //word_shift = ((uint64_t)word << )
        //ra_shift = ((uint64_t)word << 55) >> 61;
        //ra = ra_shift;
        //word = Bitpack_newu(word, CHAR_SIZE, lsb, c);
        //word 
        unsigned hi = 8 + lsb;
        word = (((uint64_t)word >> hi) << hi)
                | (((uint64_t)word << (64 - lsb)) >> (64 - lsb))
                | ((int64_t)c << lsb);
    }

    return word;
}