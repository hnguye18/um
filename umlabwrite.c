/*
 * linked to umlab.c to produce .um files for unit tests.
 * Includes all .um unit tests we used to test our um.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "fmt.h"
#include "seq.h"

extern void Um_write_sequence(FILE *output, Seq_T instructions);

extern void emit_halt_test(Seq_T instructions);
extern void emit_verbose_halt_test(Seq_T instructions);
extern void emit_add_test(Seq_T instructions);
extern void emit_print_six_test(Seq_T instructions);
extern void emit_multiply_test(Seq_T instructions);
extern void emit_divide_test(Seq_T instructions);
extern void emit_conditional_move_test(Seq_T instructions);
extern void emit_segmented_store_load_test(Seq_T instructions);
extern void emit_load_program(Seq_T instructions);
extern void emit_load_and_output_test(Seq_T instructions);
extern void emit_input_test(Seq_T instructions);
extern void emit_verbose_add_test(Seq_T instructions);
extern void emit_segments_test(Seq_T instructions);
extern void emit_nand_test(Seq_T instructions);
extern void emit_arithmetic_test(Seq_T instructions);

/* The array `tests` contains all unit tests for the lab. */

static struct test_info {
        const char *name;
        const char *test_input;          /* NULL means no input needed */
        const char *expected_output;
        /* writes instructions into sequence */
        void (*emit_test)(Seq_T stream);
} tests[] = {
        { "halt",         NULL, "", emit_halt_test },
        { "halt-verbose", NULL, "", emit_verbose_halt_test },
        { "add", NULL, "", emit_add_test },
        { "print-six", NULL, "", emit_print_six_test },
        { "multiply", NULL, "", emit_multiply_test },
        { "divide", NULL, "", emit_divide_test },
        { "conditional", NULL, "", emit_conditional_move_test },
        { "seg-load-store", NULL, "", emit_segmented_store_load_test },
        { "load-program", NULL, "", emit_load_program },
        { "load_and_output", NULL, "", emit_load_and_output_test },
        { "input", NULL, "", emit_input_test },
        { "add-verbose", NULL, "", emit_verbose_add_test },
        { "segments", NULL, "", emit_segments_test },
        { "nand", NULL, "", emit_nand_test },
        { "arithmetic", NULL, "", emit_arithmetic_test }
};
  
#define NTESTS (sizeof(tests)/sizeof(tests[0]))

/*
 * open file 'path' for writing, then free the pathname;
 * if anything fails, checked runtime error
 */
static FILE *open_and_free_pathname(char *path);

/*
 * if contents is NULL or empty, remove the given 'path', 
 * otherwise write 'contents' into 'path'.  Either way, free 'path'.
 */
static void write_or_remove_file(char *path, const char *contents);

static void write_test_files(struct test_info *test);


int main (int argc, char *argv[])
{
        bool failed = false;
        if (argc == 1)
                for (unsigned i = 0; i < NTESTS; i++) {
                        printf("***** Writing test '%s'.\n", tests[i].name);
                        write_test_files(&tests[i]);
                }
        else
                for (int j = 1; j < argc; j++) {
                        bool tested = false;
                        for (unsigned i = 0; i < NTESTS; i++)
                                if (!strcmp(tests[i].name, argv[j])) {
                                        tested = true;
                                        write_test_files(&tests[i]);
                                }
                        if (!tested) {
                                failed = true;
                                fprintf(stderr,
                                        "***** No test named %s *****\n",
                                        argv[j]);
                        }
                }
        return failed; /* failed nonzero == exit nonzero == failure */
}


static void write_test_files(struct test_info *test)
{
        FILE *binary = open_and_free_pathname(Fmt_string("%s.um", test->name));
        Seq_T instructions = Seq_new(0);
        test->emit_test(instructions);
        Um_write_sequence(binary, instructions);
        Seq_free(&instructions);
        fclose(binary);

        write_or_remove_file(Fmt_string("%s.0", test->name),
                             test->test_input);
        write_or_remove_file(Fmt_string("%s.1", test->name),
                             test->expected_output);
}


static void write_or_remove_file(char *path, const char *contents)
{
        if (contents == NULL || *contents == '\0') {
                remove(path);
        } else {
                FILE *input = fopen(path, "wb");
                assert(input != NULL);

                fputs(contents, input);
                fclose(input);
        }
        free(path);
}


static FILE *open_and_free_pathname(char *path)
{
        FILE *fp = fopen(path, "wb");
        assert(fp != NULL);

        free(path);
        return fp;
}
