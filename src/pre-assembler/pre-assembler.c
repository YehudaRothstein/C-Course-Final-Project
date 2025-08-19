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

/* פונקציות עזר */
static FILE *openInputFile(const char *inputPath);
static FILE *openOutputFile(const char *outputPath, FILE *inputFile);
static int processLines(FILE *inputFile, FILE *outputFile);
static int handleLine(char *line, int *inMacro, char *macroName, char *macroContent, int lineNumber);

/* בודק אם המחרוזת מסתיימת בהרחבת .as */
static int ends_with_as_ext(const char *s) {
    size_t n = strlen(s);
    return (n >= 3 && s[n-3]=='.' && s[n-2]=='a' && s[n-1]=='s');
}

/* הפונקציה הראשית של המעבר המוקדם */
int runPreAssembler(const char *inputPath, const char *outputPath, char *macroOutPathOut, size_t macroOutPathOutSize) {
    FILE *inputFile;
    FILE *outputFile;
    int result;
    char normalized[512]; /* מחרוזת נורמלית */

    /*  של נתיב הקלט: הוספת .as אם חסר */
    if (ends_with_as_ext(inputPath)) {
        strncpy(normalized, inputPath, sizeof(normalized)-1);
        normalized[sizeof(normalized)-1] = '\0';
    } else {
        /* הוספת .as */
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

    /* פתיחת קבצי הקלט/פלט */
    inputFile = openInputFile(normalized);
    if (!inputFile) return ERR_FILE_NOT_FOUND;

    outputFile = openOutputFile(outputPath, inputFile);
    if (!outputFile) return ERR_OUTPUT_FOLDER_MISSING;

    printf("assembling: %s -> %s\n", normalized, outputPath);

    result = processLines(inputFile, outputFile);

    fclose(inputFile);
    fclose(outputFile);

    if (macroOutPathOut && macroOutPathOutSize > 0) {
        expand_macros_file(normalized, macroOutPathOut);
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

/* עיבוד שורות הקלט */
static int processLines(FILE *inputFile, FILE *outputFile) {
    char line[LINE_SIZE];
    char macroName[MAX_MACRO_NAME];
    char macroContent[MAX_MACRO_CONTENT];
    int inMacro = 0;
    int lineNumber = 0;
    int status;

    macroName[0] = '\0';
    macroContent[0] = '\0';

    /* קריאת שורות הקלט */
    while (fgets(line, LINE_SIZE, inputFile)) {
        lineNumber++;
        /* מעבד את השורה */
        status = handleLine(line, &inMacro, macroName, macroContent, lineNumber);
        /* בדוק אם הייתה שגיאה */
        if (status != ERR_NONE) {
            return status;
        }

        /* אם לא בתוך מקרו, הדפס את המקרו */
        if (!inMacro && macroName[0] != '\0') {
            fprintf(outputFile, "\n%s:\n%s\n", macroName, macroContent);
            macroName[0] = '\0';
            macroContent[0] = '\0';
        }
    }

    /* אם יש מקרו פתוח, דווח על שגיאה */
    if (inMacro) {
        handleError(ERR_UNEXPECTED_END_OF_FILE, NULL);
        return ERR_UNEXPECTED_END_OF_FILE;
    }

    return ERR_NONE;
}

/* עיבוד שורה בודדת */
static int handleLine(char *line, int *inMacro, char *macroName, char *macroContent, int lineNumber) {
    char *t = ltrim(line);
    size_t cur, add, cap;

    /* התחלת מאקרו: */
    if (!(*inMacro) && startsWithIgnoreCase(t, "mcro") && isspace((unsigned char)t[4])) {
        if (sscanf(t + 4, "%s", macroName) == 1) {
            *inMacro = 1;
            macroContent[0] = '\0';
            return ERR_NONE;
        }
        handleError(ERR_INVALID_MACRO_SYNTAX, t);
        return ERR_INVALID_MACRO_SYNTAX;
    }

    /* סיום מאקרו */
    if (*inMacro && startsWithIgnoreCase(t, "mcroend")) {
        *inMacro = 0;
        return ERR_NONE;
    }

    /* צבירת שורות תוכן המאקרו */
    if (*inMacro) {
        cap = (size_t)MAX_MACRO_CONTENT - 1; /* השאר מקום ל־NUL */
        cur = strlen(macroContent);
        add = strlen(t);

        /* הסר CR/LF בסוף השורה */
        while (add && (t[add - 1] == '\r' || t[add - 1] == '\n')) {
            add--;
        }

        if (add && cur < cap) {
            size_t n = cap - cur;
            if (add < n) n = add;
            memcpy(macroContent + cur, t, n);
            cur += n;
            macroContent[cur] = '\0';
        }

        /* הוסף שורה חדשה אם עוד יש מקום */
        if (cur < cap) {
            macroContent[cur++] = '\n';
            macroContent[cur] = '\0';
        }
    }

    return ERR_NONE;
}
