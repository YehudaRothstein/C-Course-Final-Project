#include <stdio.h>
#include <stdlib.h>
#include "pre-assembler/pre-assembler.h"

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


    return 0;
}
