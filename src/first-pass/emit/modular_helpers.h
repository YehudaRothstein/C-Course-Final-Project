#ifndef MODULAR_HELPERS_H
#define MODULAR_HELPERS_H

#include "utils.h"
#include "code_conversion.h"
#include "label_table.h"
#include "other_table.h"

int encode_instruction(InstParts *inst, code_conv_t *code, int code_count, int line_num, LabelNode **label_table_head, int *error_found, const char *file_name);
int emit_instruction(InstParts *inst, code_conv_t *code, int code_count, int line_num, LabelNode **label_table_head, int *error_found, const char *file_name);
int regnum(const char *reg);
int mat_reg(const char *mat, int which);
void mat_label(const char *mat, char *out);

#endif
