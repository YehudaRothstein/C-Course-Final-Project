#include <stdio.h>
#include <stdlib.h>
#include "pre-assembler/pre-assembler.h"
#include "first-run/first-run.h"

int main(int argc, char *argv[]) {
    
    const char *inputFile;
    const char *outputFile;
    char macroSpreadFile[256];
    int result;
    int firstPassResult;

    if (argc < 2) {
        printf("Usage: %s <input.asm>\n", argv[0]);
        return 1;
    }

    inputFile = argv[1];
    outputFile = "outputs/macros.txt";

    result = runPreAssembler(inputFile, outputFile, macroSpreadFile, sizeof(macroSpreadFile));
    if (result != 0) {
        printf("no result.\n");
        return result;
    }

    
    firstPassResult = exe_first_pass(macroSpreadFile);
    if (firstPassResult != 0) {
        printf("First pass failed.\n");
        return firstPassResult;
    }

    return 0;
}
