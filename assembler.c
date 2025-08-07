#include <stdio.h>
#include <stdlib.h>
#include "pre-assembler/pre-assembler.h"
#include "first-run/first-run.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input.asm>\n", argv[0]);
        return 1;
    }

    const char *inputFile = argv[1];
    const char *outputFile = "outputs/macros.txt";

    char macroSpreadFile[256];
    int result = runPreAssembler(inputFile, outputFile, macroSpreadFile, sizeof(macroSpreadFile));
    if (result != 0) {
        printf("no result.\n");
        return result;
    }

    // Run first pass on the macro-spread .as file
    int firstPassResult = exe_first_pass(macroSpreadFile);
    if (firstPassResult != 0) {
        printf("First pass failed.\n");
        return firstPassResult;
    }

    return 0;
}
