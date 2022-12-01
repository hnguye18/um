/*
 * Interface for the segmented memory of the UM implementation 
 *
 */

#include <stdint.h>

#ifndef MEMORY_H_
#define MEMORY_H_

/* Pointer to a struct that contains the data structure for this module */
typedef struct Memory_T *Memory_T;

/* Creates/frees memory associated with a Memory_T */
Memory_T memory_new(uint32_t length);
void memory_free(Memory_T *m);

/* Allows user to interact with Memory_T data */
void memory_put(Memory_T m, uint32_t seg, uint32_t off, uint32_t val);
uint32_t memory_get(Memory_T m, uint32_t seg, uint32_t off);

/* Maps and Unmaps segments to Memory_T sequence */
uint32_t memory_map(Memory_T m, uint32_t length);
void     memory_unmap(Memory_T m, uint32_t seg_num);

#endif