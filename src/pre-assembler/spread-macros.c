#include <ctype.h>
#include <string.h>


void trim_whitespace(char *str) {
    char *end;
    
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
}
#include "spread-macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define LINE_SIZE 256
#define MAX_MACROS 100
#define MAX_MACRO_NAME 100
#define MAX_MACRO_CONTENT 4096

typedef struct {
    char name[MAX_MACRO_NAME];
    char content[MAX_MACRO_CONTENT];
} Macro;


static int equals_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static int starts_with_ignore_case(const char *str, const char *prefix) {
    while (*prefix && *str) {
        char cs = (char)tolower((unsigned char)*str);
        char cp = (char)tolower((unsigned char)*prefix);
        if (cs != cp) return 0;
        str++; prefix++;
    }
    return *prefix == '\0';
}

static int macro_name_match(const char *line, const char *macro_name) {
    char trimmed_line[LINE_SIZE];

    strncpy(trimmed_line, line, LINE_SIZE - 1);
    trimmed_line[LINE_SIZE - 1] = '\0';
    trim_whitespace(trimmed_line);
    
    return equals_ignore_case(trimmed_line, macro_name);
}

int spreadMacros(const char *inputPath, const char *outputPath) {
    Macro macros[MAX_MACROS];
    int macroCount = 0;

    FILE *input = fopen(inputPath, "r");
    FILE *output;
    char line[LINE_SIZE];
    int inMacroDef = 0;

    if (!input) {
        printf("Failed to open input file: %s\n", inputPath);
        return 1;
    }

    output = fopen(outputPath, "w");
    if (!output) {
        fclose(input);
        printf("Failed to create output file: %s\n", outputPath);
        return 1;
    }

    while (fgets(line, LINE_SIZE, input)) {
        char trimmed[LINE_SIZE];
        char match_line[LINE_SIZE];
        char def_check[LINE_SIZE];
        size_t len;
        int i = 0;
        int j = 0;
        int found;
        int m;

        while (line[i] && (line[i] == ' ' || line[i] == '\t')) i++;
        while (line[i]) trimmed[j++] = line[i++];
        trimmed[j] = '\0';

        
        strncpy(match_line, trimmed, LINE_SIZE - 1);
        match_line[LINE_SIZE - 1] = '\0';
        len = strlen(match_line);
        if (len > 0 && match_line[len - 1] == '\n') match_line[len - 1] = '\0';

        
        strncpy(def_check, trimmed, LINE_SIZE - 1);
        def_check[LINE_SIZE - 1] = '\0';
        trim_whitespace(def_check);

        if (!inMacroDef && starts_with_ignore_case(def_check, "mcro ")) {
            inMacroDef = 1;
            continue;
        }
        if (inMacroDef && equals_ignore_case(def_check, "mcroend")) {
            inMacroDef = 0;
            continue;
        }
        if (inMacroDef) continue;

        
        found = 0;
        for (m = 0; m < macroCount; m++) {
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
