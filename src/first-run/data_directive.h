#ifndef DATA_DIRECTIVE_H
#define DATA_DIRECTIVE_H

#define MAX_LINE_LENGTH 256

typedef struct {
    char type[8];
    char label[32];
    char operands[MAX_LINE_LENGTH];
    int src_line;
} DataDirective;

#endif
