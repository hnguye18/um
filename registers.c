/*
 * Implementation of the UM registers, which includes functions for
 * allocating/deallocating memory as well as for interactions between the
 * client and the data structure
 *
 */

#include <assert.h>
#include <stdlib.h>

#include "registers.h"
#include "uarray.h"

#define REGISTER_LEN 8

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