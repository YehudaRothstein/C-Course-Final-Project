#ifndef CODE_CONVERSION_H
#define CODE_CONVERSION_H

#define MAX_OPCODE_NAME 10

typedef struct {
    char name[MAX_OPCODE_NAME];
    int code;
    int num_operands;
    int valid_src_addr[4];
    int valid_dst_addr[4];
} OpcodeInfo;

extern OpcodeInfo opcode_table[];
extern int num_opcodes;

const OpcodeInfo* find_opcode(const char *name);
void to_binary_str(unsigned short value, char *out, int bits);
char get_are_char(int are);

#endif