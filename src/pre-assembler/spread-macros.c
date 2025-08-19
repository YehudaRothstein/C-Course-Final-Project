/* Keep includes minimal and ANSI-compliant */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spread-macros.h"

#define LINE_SIZE 256
#define MAX_MACROS 100
#define MAX_MACRO_NAME 100
#define MAX_MACRO_CONTENT 8192

typedef struct {
    char name[MAX_MACRO_NAME];
    char content[MAX_MACRO_CONTENT];
} Macro;

/* Trim both ends in-place (ASCII whitespace) */
static void trim_whitespace(char *str) {
    char *p = str;
    char *end;
    if (!str) return;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != str) memmove(str, p, strlen(p) + 1);
    if (*str == '\0') return;
    end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
}

static int ieq(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static int starts_ign_case(const char *s, const char *prefix) {
    while (*prefix && *s) {
        char cs = (char)tolower((unsigned char)*s);
        char cp = (char)tolower((unsigned char)*prefix);
        if (cs != cp) return 0;
        s++; prefix++;
    }
    return *prefix == '\0';
}

int spreadMacros(const char *inputPath, const char *outputPath) {
    Macro macros[MAX_MACROS];
    int macroCount = 0;
    int inDef = 0;
    char curName[MAX_MACRO_NAME];
    char curContent[MAX_MACRO_CONTENT];
    size_t curLen = 0;

    FILE *in = fopen(inputPath, "r");
    FILE *out;
    char line[LINE_SIZE];

    if (!in) {
        printf("Failed to open input file: %s\n", inputPath);
        return 1;
    }
    out = fopen(outputPath, "w");
    if (!out) {
        fclose(in);
        printf("Failed to create output file: %s\n", outputPath);
        return 1;
    }

    /* First pass over input: collect macro definitions and build expanded output */
    while (fgets(line, sizeof(line), in)) {
        char original[LINE_SIZE];
        char trimmed[LINE_SIZE];
        char *p;
        size_t n;
        int i;

        /* preserve original line for copying when not a macro invocation */
        strncpy(original, line, sizeof(original)-1);
        original[sizeof(original)-1] = '\0';

        /* Build trimmed (no leading/trailing whitespace, strip trailing \n) */
        strncpy(trimmed, line, sizeof(trimmed)-1);
        trimmed[sizeof(trimmed)-1] = '\0';
        /* strip CR/LF */
        n = strlen(trimmed);
        while (n && (trimmed[n-1] == '\n' || trimmed[n-1] == '\r')) trimmed[--n] = '\0';
        trim_whitespace(trimmed);

        if (!inDef && starts_ign_case(trimmed, "mcro ")) {
            /* begin macro definition */
            const char *nameStart = trimmed + 4; /* after 'mcro' */
            while (*nameStart && isspace((unsigned char)*nameStart)) nameStart++;
            if (*nameStart == '\0') {
                /* invalid macro name: skip definition block */
                inDef = 1; /* still consume until mcroend */
                curName[0] = '\0';
                curLen = 0; curContent[0] = '\0';
            } else {
                /* read name (up to whitespace) */
                size_t k = 0;
                while (nameStart[k] && !isspace((unsigned char)nameStart[k]) && k < sizeof(curName)-1) {
                    curName[k] = nameStart[k];
                    k++;
                }
                curName[k] = '\0';
                inDef = 1;
                curLen = 0; curContent[0] = '\0';
            }
            continue;
        }
        if (inDef) {
            if (ieq(trimmed, "mcroend")) {
                /* end macro definition: store if valid */
                if (curName[0] && macroCount < MAX_MACROS) {
                    strncpy(macros[macroCount].name, curName, sizeof(macros[macroCount].name)-1);
                    macros[macroCount].name[sizeof(macros[macroCount].name)-1] = '\0';
                    strncpy(macros[macroCount].content, curContent, sizeof(macros[macroCount].content)-1);
                    macros[macroCount].content[sizeof(macros[macroCount].content)-1] = '\0';
                    macroCount++;
                }
                inDef = 0;
                curName[0] = '\0';
                curLen = 0; curContent[0] = '\0';
            } else {
                /* accumulate content (keep original text including indentation) */
                size_t addLen = strlen(original);
                /* ensure there's a trailing newline in content */
                if (addLen && original[addLen-1] != '\n') {
                    if (addLen + 1 < sizeof(original)) { original[addLen++] = '\n'; original[addLen] = '\0'; }
                }
                if (curLen + addLen < sizeof(curContent)) {
                    memcpy(curContent + curLen, original, addLen);
                    curLen += addLen;
                    curContent[curLen] = '\0';
                }
            }
            continue;
        }

        /* Not in a definition: if this line calls a macro (after trimming leading ws) output its body, else copy line */
        p = trimmed;
        if (*p == '\0' || *p == ';') {
            fputs(original, out);
            continue;
        }
        for (i = 0; i < macroCount; i++) {
            if (ieq(p, macros[i].name)) {
                fputs(macros[i].content, out);
                break;
            }
        }
        if (i == macroCount) {
            fputs(original, out);
        }
    }

    fclose(in);
    fclose(out);
    printf("Spread macros to: %s\n", outputPath);
    return 0;
}
