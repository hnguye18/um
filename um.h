/*
 * Interface for the UM implementation
 *
 */

#include <stdint.h>
#include "seq.h"

#ifndef UM_H_
#define UM_H_

/* Pointer to a struct that contains the data structure for this module */
typedef struct UM_T *UM_T;

/* Contains the indices associated with specific opcodes */
typedef enum Um_opcode {
    CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
    NAND, HALT, MAP, UNMAP, OUT, IN, LOADP, LV
} Um_opcode;

/* Pointer to a struct that contains the data structure for this module */
typedef struct Registers_T *Registers_T;

/* Pointer to a struct that contains the data structure for this module */
typedef struct Memory_T *Memory_T;

/* Struct definition of a Memory_T which 
   contains two sequences: 
   - one holding pointers to UArray_T's representing segments
   - one holding pointers to uint32_t's representing free segments */
struct Memory_T {
        Seq_T segments;
        Seq_T free;
};

/* Struct definition of a UM_T which 
   contains two structs: 
   - Register_T representing the registers
   - Memory_T representing segmented memory */
struct UM_T {
    Registers_T reg;
    Memory_T mem;
};

/* Creates/frees memory associated with a Registers_T */
Registers_T registers_new();
void registers_free(Registers_T *r);

/* Allows user to interact with Registers_T data */
void registers_put(Registers_T r, uint32_t num_register, uint32_t value);
uint32_t registers_get(Registers_T r, uint32_t num_register);

/* Creates/frees memory associated with a UM_T */
UM_T um_new(uint32_t length);
void um_free(UM_T *um);

/* Executes passed in program */
void um_execute(UM_T um);
void instruction_call(UM_T um, Um_opcode op, uint32_t ra, 
              uint32_t rb, uint32_t rc);
//void populate(UM_T um, uint32_t index, uint32_t word);

/* Creates/frees memory associated with a Memory_T */
Memory_T memory_new(uint32_t length);
void memory_free(Memory_T *m);

/* Allows user to interact with Memory_T data */
void memory_put(Memory_T m, uint32_t seg, uint32_t off, uint32_t val);
uint32_t memory_get(Memory_T m, uint32_t seg, uint32_t off);

/* Maps and Unmaps segments to Memory_T sequence */
uint32_t memory_map(Memory_T m, uint32_t length);
void     memory_unmap(Memory_T m, uint32_t seg_num);

uint32_t  load_program(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);


/* Instructions */

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
static inline void populate(UM_T um, uint32_t index, uint32_t word)
{
    assert(um != NULL);
    memory_put(um->mem, 0, index, word);
}

//void      conditional_move(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
 /* Name: conditional_move
 * Input: UM_T struct; uint32_t's reprsenting registers A, B, C
 * Output: N/A
 * Does: if value in rc is not 0, moves value of rb into ra
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if any register number is valid
 */
static inline void conditional_move(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void segmented_load(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void segmented_store(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void add(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void multiply(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void divide(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void nand(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void halt(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);
    
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
static inline void map_segment(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void unmap_segment(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void output(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc)
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

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
static inline void input(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc) 
{
    assert(um != NULL);
    assert(ra < 8 && rb < 8 && rc < 8);

    int character = fgetc(stdin);

    if (character == EOF) {
        registers_put(um->reg, rc, ~0);
    }

    registers_put(um->reg, rc, character);
}

/* Name: load_value
 * Input: a UM_T struct, a register number, and a value
 * Output: N/A
 * Does: loads the passed in value into register ra
 * Error: Asserts if UM_T struct is NULL
 *        Asserts if register is invalid
 */
static inline void load_value(UM_T um, uint32_t ra, uint32_t val)
{
    assert(um != NULL);
    assert(ra < 8);

    registers_put(um->reg, ra, val);
}

// void      segmented_load(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      segmented_store(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      add(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      multiply(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      divide(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      nand(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      halt(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      map_segment(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      unmap_segment(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      output(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      input(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
// void      load_value(UM_T um, uint32_t ra, uint32_t val);





#endif