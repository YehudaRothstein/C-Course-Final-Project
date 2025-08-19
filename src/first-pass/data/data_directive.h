#ifndef DATA_DIRECTIVE_H
#define DATA_DIRECTIVE_H

#define MAX_LINE_LENGTH 256

/* מבנה נתונים לשמירת מידע על הוראות נתונים */
typedef struct DataParts {
    char type[8];
    char label[32];
    char operands[MAX_LINE_LENGTH];
    int src_line; /* מספר השורה בקובץ המקור */
} DataParts;

#endif