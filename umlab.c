/* 
 * The functions defined in this lab should be linked against umlabwrite.c
 * to produce a unit test writing program. Any additional functions and unit
 * tests written for the lab go here.
 */

#include <stdint.h>
#include <stdio.h>

#include <assert.h>
#include <seq.h>
#include <bitpack.h>

#define WORD_SIZE 32
#define CHAR_SIZE  8
#define OPCODE_LSB 28
#define RA_LSB 6
#define RB_LSB 3
#define RC_LSB 0

typedef uint32_t Um_instruction;
typedef enum Um_opcode {
    CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
    NAND, HALT, ACTIVATE, INACTIVATE, OUT, IN, LOADP, LV
} Um_opcode;


/* Functions that return the two instruction types */

Um_instruction three_register(Um_opcode op, int ra, int rb, int rc)
{
    Um_instruction word = 0;
    word = Bitpack_newu(word, 4, OPCODE_LSB, op);
    word = Bitpack_newu(word, 3, RA_LSB, ra);
    word = Bitpack_newu(word, 3, RB_LSB, rb);
    word = Bitpack_newu(word, 3, RC_LSB, rc);

    return word;
}

Um_instruction loadval(unsigned ra, unsigned val)
{
    Um_instruction word = 0;
    word = Bitpack_newu(word, 4, OPCODE_LSB, LV);
    word = Bitpack_newu(word, 3, 25, ra);
    word = Bitpack_newu(word, 25, 0, val);

    return word;
}

/* Wrapper functions for each of the instructions */

typedef enum Um_register { r0 = 0, r1, r2, r3, r4, r5, r6, r7 } Um_register;

static inline Um_instruction halt(void) 
{
    return three_register(HALT, 0, 0, 0);
}

static inline Um_instruction add(Um_register a, Um_register b, Um_register c) 
{
    return three_register(ADD, a, b, c);
}

static inline Um_instruction multiply(Um_register a, Um_register b, 
                                      Um_register c) 
{
    return three_register(MUL, a, b, c);
}

static inline Um_instruction divide(Um_register a, Um_register b, 
                                    Um_register c) 
{
    return three_register(DIV, a, b, c);
}

static inline Um_instruction conditional_move(Um_register a, Um_register b, 
                                              Um_register c)
{
    return three_register(CMOV, a, b, c);
}

static inline Um_instruction nand(Um_register a, Um_register b, 
                                                 Um_register c)
{
    return three_register(NAND, a, b, c);
}

Um_instruction output(Um_register c)
{
    return three_register(OUT, 0 ,0, c);
}

Um_instruction input(Um_register c)
{
    return three_register(IN, 0 ,0, c);
}

Um_instruction mapseg(Um_register a, Um_register b, Um_register c) 
{
    return three_register(ACTIVATE, a, b, c);
}

Um_instruction unmap(Um_register a, Um_register b, Um_register c)
{
    return three_register(INACTIVATE, a, b, c);
}

Um_instruction segload(Um_register a, Um_register b, Um_register c) 
{
    return three_register(SLOAD, a, b, c);
}

Um_instruction segstore(Um_register a, Um_register b, Um_register c) 
{
    return three_register(SSTORE, a, b, c);
}

Um_instruction load_program(Um_register a, Um_register b, Um_register c)
{
    return three_register(LOADP, a, b, c);
}

/* Functions for working with streams */

static inline void emit(Seq_T stream, Um_instruction inst)
{
    assert(sizeof(inst) <= sizeof(uintptr_t));
    Seq_addhi(stream, (void *)(uintptr_t)inst);
}

extern void Um_write_sequence(FILE *output, Seq_T stream)
{
    assert(output != NULL);
    assert(stream != NULL);

    int len = Seq_length(stream);
    for (int i = 0; i < len; i++) {
        uintptr_t aux = (uintptr_t)Seq_get(stream, i);
        Um_instruction word = (Um_instruction)aux;

        for (int lsb = WORD_SIZE - CHAR_SIZE; lsb >= 0; lsb -= CHAR_SIZE) {
            fputc(Bitpack_getu(word, CHAR_SIZE, lsb), output);
        }
    }
}

/* Unit tests for the UM */

void emit_halt_test(Seq_T stream)
{
    emit(stream, halt());
}

void emit_verbose_halt_test(Seq_T stream)
{
    emit(stream, halt());
    emit(stream, loadval(r1, 'B'));
    emit(stream, output(r1));
    emit(stream, loadval(r1, 'a'));
    emit(stream, output(r1));
    emit(stream, loadval(r1, 'd'));
    emit(stream, output(r1));
    emit(stream, loadval(r1, '!'));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

void emit_add_test(Seq_T stream)
{
    emit(stream, loadval(r2, '5'));
    emit(stream, output(r2));
    emit(stream, loadval(r4, '+'));
    emit(stream, output(r4));
    emit(stream, loadval(r3, '2'));
    emit(stream, output(r3));
    emit(stream, loadval(r5, '='));
    emit(stream, output(r5));
    emit(stream, loadval(r3, 2));
    emit(stream, add(r1, r2, r3));
    emit(stream, output(r1));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

void emit_print_six_test(Seq_T stream)
{
    emit(stream, loadval(r1, 48));
    emit(stream, loadval(r2,  6));
    emit(stream, add(r3, r1, r2));
    emit(stream, output(r3));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test multiply */
void emit_multiply_test(Seq_T stream)
{
    emit(stream, loadval(r0, 4));
    emit(stream, loadval(r1, 5));
    emit(stream, loadval(r2, 3));
    emit(stream, multiply(r3, r0, r1));
    emit(stream, multiply(r3, r3, r2));
    emit(stream, output(r3));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test divide */
void emit_divide_test(Seq_T stream)
{
    emit(stream, loadval(r0, 200));
    emit(stream, loadval(r1,   7));
    emit(stream, loadval(r2,  30));

    emit(stream, divide(r3, r0, r1));
    emit(stream, add(r3, r3, r2));
    emit(stream, output(r3));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test conditional move */
void emit_conditional_move_test(Seq_T stream)
{
    emit(stream, loadval(r1,  54));
    emit(stream, loadval(r2, 118));

    emit(stream, conditional_move(r2, r1, r0));
    emit(stream, output(r2));
    emit(stream, loadval(r0, 10));
    emit(stream, output(r0));
    emit(stream, conditional_move(r2, r1, r0));
    emit(stream, output(r2));

    /* Outputting 10 (newline) and halting */
    emit(stream, output(r0));
    emit(stream, halt());   
}

/* Test segmented load and store */
void emit_segmented_store_load_test(Seq_T stream)
{
    emit(stream, loadval(r1,  115));
    emit(stream, loadval(r2,   37));
    emit(stream, loadval(r3,  456));
    emit(stream, loadval(r4, 5045));
    emit(stream, mapseg(r0, r0, r4));

    emit(stream, segstore(r0, r2, r4));
    emit(stream, segstore(r0, r3, r1));

    emit(stream, segload(r6, r0, r2));
    emit(stream, segload(r7, r0, r3));

    emit(stream, loadval(r5, 100));
    emit(stream, divide(r6, r6, r5));
    emit(stream, output(r6));
    emit(stream, output(r7));
    emit(stream, unmap(r6, r7, r0));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test load program */
void emit_load_program(Seq_T stream)
{
    emit(stream, loadval(r1, 10));
    emit(stream, loadval(r2, (13 << 21)));
    emit(stream, loadval(r3,  (1 << 7)));
    emit(stream, mapseg(r0, r7, r1));
    
    emit(stream, multiply(r4, r2, r3)); // r4 holds the op code loadval
    emit(stream, loadval(r5, 54));
    emit(stream, add(r1, r4, r5));
    emit(stream, loadval(r6, 0));
    emit(stream, segstore(r7, r6, r1));

    emit(stream, loadval(r2, (10 << 21)));
    emit(stream, multiply(r1, r2, r3));
    emit(stream, loadval(r6, 1));
    emit(stream, segstore(r7, r6, r1));

    emit(stream, loadval(r2, (13 << 21)));
    emit(stream, multiply(r4, r2, r3)); // r4 holds the op code loadval
    emit(stream, loadval(r5, 10));
    emit(stream, add(r1, r4, r5));
    emit(stream, loadval(r6, 2));
    emit(stream, segstore(r7, r6, r1));

    emit(stream, loadval(r2, (10 << 21)));
    emit(stream, multiply(r1, r2, r3));
    emit(stream, loadval(r6, 3));
    emit(stream, segstore(r7, r6, r1));

    emit(stream, loadval(r2, (7 << 21)));
    emit(stream, multiply(r1, r2, r3));
    emit(stream, loadval(r6, 4));
    emit(stream, segstore(r7, r6, r1));

    emit(stream, loadval(r6, 0));
    emit(stream, load_program(r1, r7, r6));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test output and load value */
void emit_load_and_output_test(Seq_T stream)
{
    emit(stream, output(r0));
    emit(stream, output(r1));
    emit(stream, output(r2));
    emit(stream, output(r3));
    emit(stream, output(r4));
    emit(stream, output(r5));
    emit(stream, output(r6));
    emit(stream, output(r7));
    emit(stream, loadval(r0, 255));
    emit(stream, loadval(r1, 0));
    emit(stream, loadval(r2, 1));
    emit(stream, loadval(r3, 5));
    emit(stream, loadval(r4, '4'));
    emit(stream, loadval(r6, 87));
    emit(stream, loadval(r7, '7'));
    emit(stream, output(r0));
    emit(stream, output(r1));
    emit(stream, output(r2));
    emit(stream, output(r3));
    emit(stream, output(r4));
    emit(stream, output(r5));
    emit(stream, output(r6));
    emit(stream, output(r7));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test input */
void emit_input_test(Seq_T stream)
{
    emit(stream, loadval(r0, '\n'));
    emit(stream, loadval(r5, 400));
    emit(stream, output(r0));

    /* read in 6 chars from stdin or pipe in file */
    emit(stream, input(r0));
    emit(stream, input(r1));
    emit(stream, input(r2));
    emit(stream, input(r3));
    emit(stream, input(r4));
    
    emit(stream, input(r5)); /* input file makes sure r5 is EOF */
    emit(stream, loadval(r2, 1));
    emit(stream, loadval(r4, '~'));
    emit(stream, loadval(r6, '1')); /* r6 is not 0 */
    emit(stream, output(r6));

    /* ~0 + 1 is 2 ^ 32. (2 ^ 32) % (2 ^ 32) is 0, so r6 should be 0 */
    emit(stream, add(r6, r2, r5));

    emit(stream, conditional_move(r7, r5, r6)); 
    /* r7 should NOT have val of r5 */
    emit(stream, output(r7)); /* should not be val of r5 */
    emit(stream, add(r6, r6, r2));
    emit(stream, output(r6));
    emit(stream, output(r4));
    emit(stream, conditional_move(r7, r2, r6)); 
    /* r7 should have value of r2 */
    emit(stream, output(r7));
    emit(stream, output(r4));
    
    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test add */
void emit_verbose_add_test(Seq_T stream)
{
    emit(stream, loadval(r0, (1 << 25) - 1)); /* 2 ^ 25 - 1*/
    emit(stream, loadval(r1, 1));
    emit(stream, loadval(r2, '~'));
    emit(stream, add(r0, r0, r1)); /* 2 ^ 25 */
    emit(stream, add(r0, r0, r0)); /* (2 ^ 25) + (2 ^ 25) = (2 ^ 26) */
    emit(stream, add(r0, r0, r0));
    emit(stream, add(r0, r0, r0));
    emit(stream, add(r0, r0, r0));
    emit(stream, add(r0, r0, r0));
    emit(stream, add(r0, r0, r0));
    emit(stream, add(r0, r0, r0));
    emit(stream, output(r2));
    emit(stream, output(r0)); /* should print 0 */
    emit(stream, output(r2));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test map segment, segmented store, segmented load */
void emit_segments_test(Seq_T stream)
{
    emit(stream, loadval(r0, '\n'));
    emit(stream, loadval(r2, 12));
    emit(stream, loadval(r3, 10));
    emit(stream, loadval(r4, '4'));
    emit(stream, loadval(r6, 8));
    emit(stream, loadval(r7, '7'));
    emit(stream, output(r4));
    emit(stream, output(r0));

    /* map seg with len 12 and store in r1 */
    emit(stream, mapseg(r3, r1, r2));
    emit(stream, segstore(r1, r3, r4)); /* store val of r4 in $m[r1][10] */
    emit(stream, segload(r5, r1, r3)); /* load val of $m[r1][10] in r5 */
    emit(stream, output(r5)); /* should print '4' */
    emit(stream, unmap(r0, r2, r1)); /* unmap seg at val in r1 */
    
    /* Outputting newline */
    emit(stream, loadval(r0, 10));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test nand */
void emit_nand_test(Seq_T stream)
{
    emit(stream, loadval(r0, (1 << 25) - 1)); /* r0 is 2 ^ 25 - 1*/
    emit(stream, loadval(r1, 1));
    emit(stream, loadval(r2, (1 << 7))); /* r2 is 64 */
    emit(stream, loadval(r3, 14));
    emit(stream, multiply(r4, r0, r2)); /* r4 is (1 << 32) - 128 */
    emit(stream, add(r5, r4, r1)); /* r5 is (1 << 32) - 127 */
    emit(stream, add(r4, r4, r1)); /* r4 is (1 << 32) - 127 */
    emit(stream, nand(r7, r5, r4)); /* r7 is 126 */
    emit(stream, output(r7));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}

/* Test arithmetic */
void emit_arithmetic_test(Seq_T stream)
{
    /* Outputting A */
    emit(stream, loadval(r0, 33554431));
    emit(stream, loadval(r1,  5032232));
    emit(stream, loadval(r2,     2345));
    emit(stream, loadval(r3,       42));
    emit(stream, loadval(r4,        2));
    emit(stream, loadval(r6,       31));
    emit(stream, loadval(r7,      100));
    emit(stream, add(r5, r0, r1));
    emit(stream, add(r5, r5, r2));
    emit(stream, divide(r5, r5, r3));
    emit(stream, multiply(r6, r2, r7));
    emit(stream, divide(r5, r5, r6));
    emit(stream, loadval(r6, 31));
    emit(stream, multiply(r1, r6, r4));
    emit(stream, add(r5, r5, r1));
    emit(stream, output(r5));

    /* Outputting B */
    emit(stream, loadval(r0,  1));
    emit(stream, loadval(r1,  5));
    emit(stream, loadval(r2, 68));

    emit(stream, nand(r6, r0, r1));
    emit(stream, add(r6, r6, r2));
    emit(stream, output(r6));

    /* Outputting C */
    emit(stream, loadval(r1,      67));
    emit(stream, loadval(r2,     400));
    emit(stream, loadval(r3, 1 << 24));
    emit(stream, loadval(r4,     256));

    emit(stream, multiply(r6, r3, r4));
    emit(stream, conditional_move(r1, r2, r6));
    emit(stream, output(r1));

    /* Outputting Apostrophe - 44 */
    emit(stream, loadval(r0,      20));
    emit(stream, loadval(r1,     100));
    emit(stream, loadval(r2, 1 << 24));
    emit(stream, loadval(r3, 1 << 12));
    emit(stream, loadval(r4,      45));
    emit(stream, loadval(r5,      54));
    emit(stream, loadval(r6,     600));

    emit(stream, mapseg(r0, r1 , r3));
    emit(stream, segstore(r1, r4, r2));
    emit(stream, segstore(r1, r6, r3));
    emit(stream, segstore(r1, r5, r6));
    emit(stream, segload(r7, r1, r6));
    emit(stream, add(r7, r7, r4));
    emit(stream, divide(r7, r7, r5)); 
    emit(stream, nand(r7, r7, r0));
    emit(stream, segload(r4, r1, r5));
    emit(stream, conditional_move(r7, r0, r4));
    emit(stream, loadval(r4, 19));
    emit(stream, add(r7, r4, r7));
    emit(stream, output(r7));
    emit(stream, unmap(r0, r2, r1));

    /* Oututting s - 115 */
    emit(stream, loadval(r0, 429497));
    emit(stream, loadval(r1,  10000));
    emit(stream, loadval(r2,     21));
    emit(stream, loadval(r3,      9));
    emit(stream, loadval(r4,      0));
    emit(stream, loadval(r6,      5));
    emit(stream, loadval(r7,     15));

    emit(stream, multiply(r4, r0, r1));
    emit(stream, divide(r4, r4, r2));
    emit(stream, divide(r4, r4, r6));
    emit(stream, loadval(r0, 4));
    emit(stream, multiply(r4, r4, r0));
    emit(stream, add(r4, r4, r7));
    emit(stream, output(r4));

    /* Outputting newline and halting */
    emit(stream, loadval(r0, '\n'));
    emit(stream, output(r0));
    emit(stream, halt());
}
