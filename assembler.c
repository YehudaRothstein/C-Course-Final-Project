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

    int result = runPreAssembler(inputFile, outputFile);
    if (result != 0) {
        printf("no result.\n");
        return result;
    }

    // Spread macros (creates .am file)
    // spreadMacros(inputFile, outputFile); // If not already called in runPreAssembler

    // Run first pass on the .am file
    char amFile[256];
    snprintf(amFile, sizeof(amFile), "%s.am", inputFile);
    int firstPassResult = exe_first_pass(amFile);
    if (firstPassResult != 0) {
        printf("First pass failed.\n");
        return firstPassResult;
    }

    return 0;
}
