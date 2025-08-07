#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../utils/utils.h"
#include "pre-assembler.h"
#include "spread-macros.h"
#include "../error-handler/error-handler.h"

#define LINE_SIZE 256
#define MAX_MACROS 100
#define MAX_MACRO_NAME 100
#define MAX_MACRO_CONTENT 8192

/* Helper Declarations */
static FILE *openInputFile(const char *inputPath);
static FILE *openOutputFile(const char *outputPath, FILE *inputFile);
static int processLines(FILE *inputFile, FILE *outputFile);
static int handleLine(char *line, int *inMacro, char *macroName, char *macroContent, int lineNumber);

// Returns 0 on success, or error code. If macroOutPathOut is not NULL, sets it to the macro-spread file path.
int runPreAssembler(const char *inputPath, const char *outputPath, char *macroOutPathOut, size_t macroOutPathOutSize) {
    FILE *inputFile = openInputFile(inputPath);
    if (!inputFile) return ERR_FILE_NOT_FOUND;

    FILE *outputFile = openOutputFile(outputPath, inputFile);
    if (!outputFile) return ERR_OUTPUT_FOLDER_MISSING;

    printf("assembling: %s -> %s\n", inputPath, outputPath);

    int result = processLines(inputFile, outputFile);


    fclose(inputFile);
    fclose(outputFile);




    // Create the output file name for the macro-spread file (Linux only)
    char macroOutPath[512];
    const char *slash = strrchr(inputPath, '/');
    const char *base = slash ? slash + 1 : inputPath;
    // Remove extension if present
    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    snprintf(macroOutPath, sizeof(macroOutPath), "outputs/%.*s.as", (int)len, base);

    spreadMacros(inputPath, macroOutPath);

    // Delete the macros file after the program finishes
    remove("outputs/macros.txt");

    // Set the output path for the caller if requested
    if (macroOutPathOut && macroOutPathOutSize > 0) {
        strncpy(macroOutPathOut, macroOutPath, macroOutPathOutSize - 1);
        macroOutPathOut[macroOutPathOutSize - 1] = '\0';
    }

    return result;
}

static FILE *openInputFile(const char *inputPath) {
    FILE *file = fopen(inputPath, "r");
    if (!file) {
        handleError(ERR_FILE_NOT_FOUND, inputPath);
    }
    return file;
}

static FILE *openOutputFile(const char *outputPath, FILE *inputFile) {
    FILE *file = fopen(outputPath, "w");
    if (!file) {
        fclose(inputFile);
        handleError(ERR_OUTPUT_FOLDER_MISSING, outputPath);
    }
    return file;
}

static int processLines(FILE *inputFile, FILE *outputFile) {
    char line[LINE_SIZE];
    char macroName[MAX_MACRO_NAME] = "";
    char macroContent[MAX_MACRO_CONTENT] = "";
    int inMacro = 0;
    int lineNumber = 0;

    while (fgets(line, LINE_SIZE, inputFile)) {
        lineNumber++;
        int status = handleLine(line, &inMacro, macroName, macroContent, lineNumber);
        if (status != ERR_NONE) {
            return status;
        }

        if (!inMacro && macroName[0] != '\0') {
            fprintf(outputFile, "\n%s:\n%s\n", macroName, macroContent);
            macroName[0] = '\0';
            macroContent[0] = '\0';
        }
    }

    if (inMacro) {
        handleError(ERR_UNEXPECTED_END_OF_FILE, NULL);
        return ERR_UNEXPECTED_END_OF_FILE;
    }

    return ERR_NONE;
}

static int handleLine(char *line, int *inMacro, char *macroName, char *macroContent, int lineNumber) {
    char *trimmed = ltrim(line);

    if (!(*inMacro) && startsWithIgnoreCase(trimmed, "mcro") && isspace(trimmed[4])) {
        if (sscanf(trimmed + 4, "%s", macroName) == 1) {
            *inMacro = 1;
            macroContent[0] = '\0';
        } else {
            handleError(ERR_INVALID_MACRO_SYNTAX, trimmed);
            return ERR_INVALID_MACRO_SYNTAX;
        }
    } else if (*inMacro && startsWithIgnoreCase(trimmed, "mcroend")) {
        *inMacro = 0;
    } else if (*inMacro) {
        strcat(macroContent, trimmed);
        if (trimmed[strlen(trimmed) - 1] != '\n') {
            strcat(macroContent, "\n");
        }
    }

    return ERR_NONE;
}
