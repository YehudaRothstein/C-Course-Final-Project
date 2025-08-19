#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "spread-macros.h"

/* גדלים קבועים */
#define LINE_SIZE 256  
#define MAX_MACROS 100
#define MAX_MACRO_NAME 100
#define MAX_MACRO_CONTENT 8192

/* טיפוס למאקרו אחד */
typedef struct {
    char name[MAX_MACRO_NAME];
    char content[MAX_MACRO_CONTENT];
} Macro;


/* התאמה ללא תלות ברישיות */
static int equals_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

/* התחלה בקידומת ללא תלות ברישיות */
static int starts_with_ignore_case(const char *s, const char *prefix) {
    while (*s && *prefix) {
        char cs = (char)tolower((unsigned char)*s); /**/
        char cp = (char)tolower((unsigned char)*prefix);
        if (cs != cp) return 0;
        s++; prefix++;
    }
    return *prefix == '\0';
}

/* קריאת קובץ, פרישת מאקרו וכתיבה */
int expand_macros_file(const char *inputPath, const char *outputPath) {
    FILE *in;
    FILE *out;

    Macro macros[MAX_MACROS];
    int macroCount = 0;

    int inDef = 0;
    char curName[MAX_MACRO_NAME];
    char curContent[MAX_MACRO_CONTENT];
    size_t curLen = 0;

    char line[LINE_SIZE];
    char original[LINE_SIZE];
    char trimmed[LINE_SIZE];

    curName[0] = '\0';
    curContent[0] = '\0';

    in = fopen(inputPath, "r");
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

    /* מעבר על כל השורות */
    while (fgets(line, sizeof(line), in)) {
        size_t n;
        size_t k;
        int i;

        /* שמירה של המקור */
        strncpy(original, line, sizeof(original) - 1);
        original[sizeof(original) - 1] = '\0';

        /* בניית גרסה מנוקָה */
        strncpy(trimmed, line, sizeof(trimmed) - 1);

        trimmed[sizeof(trimmed) - 1] = '\0';
        n = strlen(trimmed);
        while (n && (trimmed[n - 1] == '\n' || trimmed[n - 1] == '\r')) {
            trimmed[--n] = '\0';
        }

        /* הסר רווחים מהתחלה ומהסוף */
        {
            char *p = trimmed; 
            char *q; 

            while (*p && isspace((unsigned char)*p)) p++;

            if (p != trimmed) memmove(trimmed, p, strlen(p) + 1);

            if (trimmed[0] != '\0') {
                q = trimmed + strlen(trimmed) - 1;
                while (q >= trimmed && isspace((unsigned char)*q)) q--;
                *(q + 1) = '\0';
            }
        }

        /* התחלת הגדרת מאקרו */
        if (!inDef && starts_with_ignore_case(trimmed, "mcro ")) {
            /* זיהוי שם המאקרו */
            const char *nameStart = trimmed + 4; 
            while (*nameStart && isspace((unsigned char)*nameStart)) nameStart++; 

            /*   */
            if (*nameStart == '\0') {
                inDef = 1;
                curName[0] = '\0';
                curLen = 0; curContent[0] = '\0';
            } else {
                k = 0; 
                while (nameStart[k] && !isspace((unsigned char)nameStart[k]) && k < MAX_MACRO_NAME - 1) {
                    curName[k] = nameStart[k];
                    k++;
                }
                curName[k] = '\0';
                inDef = 1;
                curLen = 0; curContent[0] = '\0';
            }
            continue; 
        }

        /* בתוך הגדרה */
        if (inDef) {
            /* הוספת תוכן למאקרו */
            if (equals_ignore_case(trimmed, "mcroend")) {
                /* מעתיקים את תוכן המאקרו */
                if (curName[0] && macroCount < MAX_MACROS) {
                    /* שמירה של שם המאקרו */
                    strncpy(macros[macroCount].name, curName, MAX_MACRO_NAME - 1);
                    macros[macroCount].name[MAX_MACRO_NAME - 1] = '\0';
                    /* שמירה של תוכן המאקרו */
                    strncpy(macros[macroCount].content, curContent, MAX_MACRO_CONTENT - 1);
                    macros[macroCount].content[MAX_MACRO_CONTENT - 1] = '\0';
                    macroCount++;
                }
                inDef = 0;
                curName[0] = '\0';
                curLen = 0; curContent[0] = '\0';
            } else {
                /* הוספת תוכן למאקרו */
                size_t addLen = strlen(original);
                if (curLen + addLen < MAX_MACRO_CONTENT) {
                    memcpy(curContent + curLen, original, addLen);
                    curLen += addLen;
                    curContent[curLen] = '\0';
                }
            }
            continue; 
        }

        /* אם השורה ריקה או מתחילה ב-; */
        if (trimmed[0] == '\0' || trimmed[0] == ';') {
            fputs(original, out);
            continue;
        }

        /* חיפוש מאקרו */
        for (i = 0; i < macroCount; i++) {
            if (equals_ignore_case(trimmed, macros[i].name)) {
                fputs(macros[i].content, out);
                break;
            }
        }

        /* אם לא נמצא מאקרו */
        if (i == macroCount) {
            fputs(original, out);
        }
    }

    fclose(in);
    fclose(out);
    printf("Spread macros to: %s\n", outputPath);
    return 0;
}
