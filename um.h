/*
 * Interface for the UM implementation
 *
 */

#include <stdint.h>

#ifndef UM_H_
#define UM_H_

/* Pointer to a struct that contains the data structure for this module */
typedef struct UM_T *UM_T;

/* Contains the indices associated with specific opcodes */
typedef enum Um_opcode {
    CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
    NAND, HALT, MAP, UNMAP, OUT, IN, LOADP, LV
} Um_opcode;

/* Creates/frees memory associated with a UM_T */
UM_T um_new(uint32_t length);
void um_free(UM_T *um);

/* Executes passed in program */
void um_execute(UM_T um);
void instruction_call(UM_T um, Um_opcode op, uint32_t ra, 
              uint32_t rb, uint32_t rc);
void populate(UM_T um, uint32_t index, uint32_t word);

/* Instructions */
void      conditional_move(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      segmented_load(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      segmented_store(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      add(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      multiply(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      divide(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      nand(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      halt(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      map_segment(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      unmap_segment(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      output(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      input(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
uint32_t  load_program(UM_T um, uint32_t ra, uint32_t rb, uint32_t rc);
void      load_value(UM_T um, uint32_t ra, uint32_t val);

#endif