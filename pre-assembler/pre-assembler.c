#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../utils/utils.h"
#include "pre-assembler.h"
#include "../error-handler/error-handler.h"

#define LINE_SIZE 256
#define MAX_MACRO_NAME 100

int runPreAssembler(const char *inputPath, const char *outputPath) {
    FILE *inputFile = fopen(inputPath, "r");
    if (!inputFile) {
        handleError(ERR_FILE_NOT_FOUND, inputPath);
        return ERR_FILE_NOT_FOUND;
    }

    FILE *outputFile = fopen(outputPath, "w");
    if (!outputFile) {
        fclose(inputFile);
        handleError(ERR_OUTPUT_FOLDER_MISSING, outputPath);
        return ERR_OUTPUT_FOLDER_MISSING;
    }

    printf("assembling: %s -> %s\n", inputPath, outputPath);

    char line[LINE_SIZE];
    char macroName[MAX_MACRO_NAME];
    int inMacro = 0;
    int lineNumber = 0;

    while (fgets(line, LINE_SIZE, inputFile)) {
        lineNumber++;
        char *trimmed = ltrim(line);
        printf("🔍 Line %d: %s", lineNumber, trimmed);

        if (!inMacro && startsWithIgnoreCase(trimmed, "mcro") && isspace(trimmed[4])) {
            if (sscanf(trimmed + 4, "%s", macroName) == 1) {
                inMacro = 1;
            } else {
                // This is now an error, not a warning
                fclose(inputFile);
                fclose(outputFile);
                handleError(ERR_INVALID_MACRO_SYNTAX, trimmed);
                return ERR_INVALID_MACRO_SYNTAX;
            }
        } else if (inMacro && startsWithIgnoreCase(trimmed, "mcroend")) {
            inMacro = 0;
        }
    }

    if (inMacro) {
        fclose(inputFile);
        fclose(outputFile);
        handleError(ERR_UNEXPECTED_END_OF_FILE, NULL);
        return ERR_UNEXPECTED_END_OF_FILE;
    }

    fclose(inputFile);
    fclose(outputFile);

    return ERR_NONE;
}
