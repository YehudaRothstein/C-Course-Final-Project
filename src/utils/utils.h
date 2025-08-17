/* C-Course Final Project - Assembler (authored by Yehuda) */
#ifndef UTILS_H
#define UTILS_H


typedef struct {
    char *label;
    char *opcode;
    char *operands;
    char *src_operand;
    char *dst_operand;
} InstParts;


char* ltrim(char* str);

int startsWithIgnoreCase(const char* str, const char* prefix);

void removeExtraSpacesStr(char *str);

void removeSpacesNextToComma(char *str);

InstParts parseInstLine(char *line);

int isInstr(const char *opcode);


void freeInstParts(InstParts *inst);

#endif
