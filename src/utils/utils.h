/* C-Course Final Project - Assembler (authored by Yehuda) */
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>


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

/* Label validation and error printing helpers */
int legal_label_decl(const char *label, int *error_code);
void print_external_error(int error_code, const char *file, int line);

/* Build base filename without extension (used for outputs) */
void get_basefile(const char *path, char *out_base, size_t out_size);

#endif
