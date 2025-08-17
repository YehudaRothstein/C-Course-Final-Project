#include <stdio.h>
#include <stdlib.h>
#include "pre-assembler/pre-assembler.h"
#include "first-run/first-run.h"

int main(int argc, char *argv[]) {
    
    const char *in_file; /* input file from command line */
    const char *outputFile;
    char macro_spread_file[256];
    int pre_assembler_result;
    int first_pass_res;

    if (argc < 2) {
        printf("Usage: %s <input.asm>\n", argv[0]);
        return 1;
    }

    in_file = argv[1];
    outputFile = "outputs/macros.txt";

    /* printf("DEBUG: Input file is %s\n", in_file); */

    pre_assembler_result = runPreAssembler(in_file, outputFile, macro_spread_file, sizeof(macro_spread_file));
    if (pre_assembler_result != 0) {
        printf("no result from pre-assembler.\n");
        return pre_assembler_result;
    }

    
    first_pass_res = exe_first_pass(macro_spread_file);
    if (first_pass_res != 0) {
        printf("First pass failed.\n");
        return first_pass_res;
    }

    return 0;
}
