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


static FILE *openInputFile(const char *inputPath);
static FILE *openOutputFile(const char *outputPath, FILE *inputFile);
static int processLines(FILE *inputFile, FILE *outputFile);
static int handleLine(char *line, int *inMacro, char *macroName, char *macroContent, int lineNumber);

/* Helper to check .as extension locally */
static int ends_with_as_ext(const char *s) {
    size_t n = strlen(s);
    return (n >= 3 && s[n-3]=='.' && s[n-2]=='a' && s[n-1]=='s');
}

int runPreAssembler(const char *inputPath, const char *outputPath, char *macroOutPathOut, size_t macroOutPathOutSize) {
    FILE *inputFile;
    FILE *outputFile;
    int result;
    char normalized[512];

    /* Normalize input path: append .as if missing */
    if (ends_with_as_ext(inputPath)) {
        strncpy(normalized, inputPath, sizeof(normalized)-1);
        normalized[sizeof(normalized)-1] = '\0';
    } else {
        size_t cap = sizeof(normalized);
        size_t inlen = strlen(inputPath);
        if (inlen >= cap) inlen = cap - 1;
        memcpy(normalized, inputPath, inlen);
        normalized[inlen] = '\0';
        if (inlen + 3 < cap) {
            normalized[inlen++] = '.';
            normalized[inlen++] = 'a';
            normalized[inlen++] = 's';
            normalized[inlen] = '\0';
        }
    }

    inputFile = openInputFile(normalized);
    if (!inputFile) return ERR_FILE_NOT_FOUND;

    outputFile = openOutputFile(outputPath, inputFile);
    if (!outputFile) return ERR_OUTPUT_FOLDER_MISSING;

    printf("assembling: %s -> %s\n", normalized, outputPath);

    result = processLines(inputFile, outputFile);

    fclose(inputFile);
    fclose(outputFile);

    /* If caller provided a target .am path, spread macros into that exact path */
    if (macroOutPathOut && macroOutPathOutSize > 0) {
        /* macroOutPathOut already contains the desired <dir>/<base>.am (built by main). */
        spreadMacros(normalized, macroOutPathOut);
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
    char macroName[MAX_MACRO_NAME];
    char macroContent[MAX_MACRO_CONTENT];
    int inMacro = 0;
    int lineNumber = 0;
    int status;

    macroName[0] = '\0';
    macroContent[0] = '\0';

    while (fgets(line, LINE_SIZE, inputFile)) {
        lineNumber++;
        status = handleLine(line, &inMacro, macroName, macroContent, lineNumber);
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

    if (!(*inMacro) && startsWithIgnoreCase(trimmed, "mcro") && isspace((unsigned char)trimmed[4])) {
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
