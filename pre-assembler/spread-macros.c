#include <ctype.h>
#include <string.h>

// Helper to trim whitespace in-place
void trim_whitespace(char *str) {
    char *end;
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
}
#include "spread-macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Macro constants
#define LINE_SIZE 256
#define MAX_MACROS 100
#define MAX_MACRO_NAME 100
#define MAX_MACRO_CONTENT 4096

typedef struct {
    char name[MAX_MACRO_NAME];
    char content[MAX_MACRO_CONTENT];
} Macro;

static int macro_name_match(const char *line, const char *macro_name) {
    char trimmed_line[LINE_SIZE];
    strncpy(trimmed_line, line, LINE_SIZE - 1);
    trimmed_line[LINE_SIZE - 1] = '\0';
    trim_whitespace(trimmed_line);
    // Compare case-insensitive
    return strcasecmp(trimmed_line, macro_name) == 0;
}

int spreadMacros(const char *inputPath, const char *outputPath) {
    Macro macros[MAX_MACROS];
    int macroCount = 0;

    FILE *input = fopen(inputPath, "r");
    if (!input) {
        printf("Failed to open input file: %s\n", inputPath);
        return 1;
    }

    FILE *output = fopen(outputPath, "w");
    if (!output) {
        fclose(input);
        printf("Failed to create output file: %s\n", outputPath);
        return 1;
    }

    char line[LINE_SIZE];
    int inMacroDef = 0;
    while (fgets(line, LINE_SIZE, input)) {
        char trimmed[LINE_SIZE];
        int i = 0, j = 0;
        while (line[i] && (line[i] == ' ' || line[i] == '\t')) i++;
        while (line[i]) trimmed[j++] = line[i++];
        trimmed[j] = '\0';

        // Remove trailing newline for matching
        char match_line[LINE_SIZE];
        strncpy(match_line, trimmed, LINE_SIZE - 1);
        match_line[LINE_SIZE - 1] = '\0';
        size_t len = strlen(match_line);
        if (len > 0 && match_line[len - 1] == '\n') match_line[len - 1] = '\0';

        // Macro definition block detection (ignore spaces/tabs)
        char def_check[LINE_SIZE];
        strncpy(def_check, trimmed, LINE_SIZE - 1);
        def_check[LINE_SIZE - 1] = '\0';
        trim_whitespace(def_check);

        if (!inMacroDef && strncasecmp(def_check, "mcro ", 5) == 0) {
            inMacroDef = 1;
            continue;
        }
        if (inMacroDef && strncasecmp(def_check, "mcroend", 7) == 0) {
            inMacroDef = 0;
            continue;
        }
        if (inMacroDef) continue;

        // Macro call detection (ignore whitespace, case-insensitive, only macro name on line)
        int found = 0;
        for (int m = 0; m < macroCount; m++) {
            if (macro_name_match(match_line, macros[m].name)) {
                fputs(macros[m].content, output);
                found = 1;
                break;
            }
        }
        if (!found) {
            fputs(line, output);
        }
    }

    fclose(input);
    fclose(output);
    printf("Spread macros to: %s\n", outputPath);
    return 0;
}
