#include "code_conversion.h"
#include <string.h>

OpcodeInfo opcode_table[] = {
    {"mov",  0, 2, {1,1,1,1}, {0,1,1,1}},
    {"cmp",  1, 2, {1,1,1,1}, {1,1,1,1}},
    {"add",  2, 2, {1,1,1,1}, {0,1,1,1}},
    {"sub",  3, 2, {1,1,1,1}, {0,1,1,1}},
    {"lea",  4, 2, {0,1,1,0}, {0,1,1,1}},
    {"clr",  5, 1, {0,0,0,0}, {0,1,1,1}},
    {"not",  6, 1, {0,0,0,0}, {0,1,1,1}},
    {"inc",  7, 1, {0,0,0,0}, {0,1,1,1}},
    {"dec",  8, 1, {0,0,0,0}, {0,1,1,1}},
    {"jmp",  9, 1, {0,0,0,0}, {1,1,1,1}},
    {"bne", 10, 1, {0,0,0,0}, {1,1,1,1}},
    {"jsr", 11, 1, {0,0,0,0}, {1,1,1,1}},
    {"red", 12, 1, {0,0,0,0}, {0,1,1,1}},
    {"prn", 13, 1, {0,0,0,0}, {1,1,1,1}},
    {"rts", 14, 0, {0,0,0,0}, {0,0,0,0}},
    {"stop",15, 0, {0,0,0,0}, {0,0,0,0}}
};
int num_opcodes = sizeof(opcode_table) / sizeof(OpcodeInfo);

const OpcodeInfo* find_opcode(const char *name) {
    for (int i = 0; i < num_opcodes; i++)
        if (strcmp(name, opcode_table[i].name) == 0)
            return &opcode_table[i];
    return NULL;
}

void to_binary_str(unsigned short value, char *out, int bits) {
    for (int i = bits-1; i >= 0; i--)
        out[bits-1-i] = ((value >> i) & 1) ? '1' : '0';
    out[bits] = '\0';
}

char get_are_char(int are) {
    switch (are) {
        case 0: return 'A';
        case 1: return 'R';
        case 2: return 'E';
        default: return '?';
    }
}