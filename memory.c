/*
 * Implementation for the UM segmented memory, which includes functions for
 * allocation/deallocation of memory, mapping/unmapping segments, and
 * interacting with segments (get/put)
 *
 */

#include <assert.h>
#include <stdlib.h>

#include "memory.h"
#include "uarray.h"
#include "seq.h"

#define HINT 10

/* Struct definition of a Memory_T which 
   contains two sequences: 
   - one holding pointers to UArray_T's representing segments
   - one holding pointers to uint32_t's representing free segments */
struct Memory_T {
        Seq_T segments;
        Seq_T free;
};

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