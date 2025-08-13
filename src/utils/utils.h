#ifndef UTILS_H
#define UTILS_H


typedef struct {
    char *label;
    char *opcode;
    char *operands;
    char *src_operand;
    char *dst_operand;
} inst_parts;


char* ltrim(char* str);

int startsWithIgnoreCase(const char* str, const char* prefix);

void remove_extra_spaces_str(char *str);

void remove_spaces_next_to_comma(char *str);

inst_parts parse_inst_line(char *line);

int is_instr(const char *opcode);


void free_inst_parts(inst_parts *inst);

#endif
