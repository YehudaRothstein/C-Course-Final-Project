#ifndef DATA_DIRECTIVE_H
#define DATA_DIRECTIVE_H

#define MAX_LINE_LENGTH 256

/* Data structure to hold data instruction info */
typedef struct DataParts {
    char type[8];
    char label[32];
    char operands[MAX_LINE_LENGTH];
    int src_line; /* Line number in the source file */
} DataParts;

#endif