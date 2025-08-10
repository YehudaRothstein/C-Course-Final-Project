#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"
#include "code_conversion.h"
#include "label_table.h"
#include "modular_helpers.h"

#ifndef IC_INIT_VALUE
#define IC_INIT_VALUE 100
#endif

// Addressing modes
// 0 = immediate (#n)
// 1 = direct label (XYZ)
// 2 = matrix a[rX][rY]
// 3 = register direct r0..r7
static int addr_mode(const char *op) {
    if (!op || !*op) return -1;
    if (op[0] == '#') return 0;
    if (op[0] == 'r' && op[1] >= '0' && op[1] <= '7' && op[2] == '\0') return 3;
    if (strchr(op, '[')) return 2; // matrix addressing
    return 1; // label
}

// Trim whitespace in-place (simple)
static void trim(char *s) {
    if (!s) return;
    char *p = s; while (isspace((unsigned char)*p)) p++; 
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}

// Split operands string into src,dst (dst may be NULL). Returns operand count.
static int split_operands(const char *operands, char *src, size_t srcsz, char *dst, size_t dstsz) {
    if (!operands || !*operands) return 0;
    const char *comma = strchr(operands, ',');
    if (!comma) {
        strncpy(src, operands, srcsz-1); src[srcsz-1] = '\0'; trim(src);
        return *src ? 1 : 0;
    }
    size_t len1 = (size_t)(comma - operands);
    if (len1 >= srcsz) len1 = srcsz - 1;
    memcpy(src, operands, len1); src[len1] = '\0'; trim(src);
    strncpy(dst, comma + 1, dstsz - 1); dst[dstsz - 1] = '\0'; trim(dst);
    return (*src ? 1 : 0) + (*dst ? 1 : 0);
}

// Pack first word: opcode in bits 6-9, src mode in 4-5, dst mode in 2-3, ARE in 0-1
static unsigned short pack_first_word(int opcode, int src_mode, int dst_mode) {
    unsigned short w = 0;
    w |= ((unsigned short)(opcode & 0xF)) << 6;           // opcode
    w |= ((unsigned short)(src_mode & 0x3)) << 4;         // src mode
    w |= ((unsigned short)(dst_mode & 0x3)) << 2;         // dst mode
    // First word is absolute (ARE=00)
    w |= 0; // ARE bits already 0
    return w & 0x3FF;
}

// Helpers already declared in header
int regnum(const char *reg) {
    if (reg && reg[0] == 'r' && reg[1] >= '0' && reg[1] <= '7' && reg[2] == '\0')
        return reg[1] - '0';
    return 0;
}

int mat_reg(const char *mat, int which) {
    const char *p = mat;
    int found = 0;
    while (*p) {
        if (*p == '[') {
            if (which == found) {
                p++;
                if (*p == 'r' && p[1] >= '0' && p[1] <= '7')
                    return p[1] - '0';
            }
            found++;
        }
        p++;
    }
    return 0;
}

void mat_label(const char *mat, char *out) {
    const char *p = strchr(mat, '[');
    if (!p) { strcpy(out, mat); return; }
    size_t len = (size_t)(p - mat);
    strncpy(out, mat, len);
    out[len] = '\0';
}

// Encode a single register word: when both operands are registers, they share one word:
// bits 6-9 = src reg, bits 2-5 = dst reg, ARE=00
static unsigned short reg_pair_word(int src_reg, int dst_reg) {
    unsigned short w = 0;
    w |= ((unsigned short)(src_reg & 0xF)) << 6;
    w |= ((unsigned short)(dst_reg & 0xF)) << 2;
    return w & 0x3FF;
}

// Add an extra word for a direct label (relocatable placeholder)
static void add_label_word(code_conv *code, int idx, const char *sym) {
    code[idx].value = 0;                  // to be resolved on 2nd pass
    code[idx].are = 1;                    // relocatable
    // store symbol name to resolve later
    if (sym && *sym) {
        strncpy(code[idx].ext_name, sym, sizeof(code[idx].ext_name)-1);
        code[idx].ext_name[sizeof(code[idx].ext_name)-1] = '\0';
    } else {
        code[idx].ext_name[0] = '\0';
    }
}

// Add an immediate word (#num): value in bits 9..2, ARE=00
static void add_immediate_word(code_conv *code, int idx, int num) {
    unsigned short w = ((unsigned short)(num & 0xFF)) << 2;
    code[idx].value = w & 0x3FF;
    code[idx].are = 0;
    code[idx].ext_name[0] = '\0';
}

// Add matrix words: first label (reloc), then reg pair word with row and col regs
static int add_matrix_words(code_conv *code, int idx, const char *operand) {
    char lbl[64];
    mat_label(operand, lbl);
    add_label_word(code, idx, lbl); // matrix base label address
    int rrow = mat_reg(operand, 0);
    int rcol = mat_reg(operand, 1);
    code[idx+1].value = reg_pair_word(rrow, rcol);
    code[idx+1].are = 0;
    code[idx+1].ext_name[0] = '\0';
    return 2;
}

int emit_instruction(inst_parts *inst, code_conv *code, int code_count, int line_num, LabelNode **label_table_head, int *error_found, const char *file_name) {
    if (!inst || !inst->opcode) return 0;

    // Insert code label if present
    if (inst->label && *inst->label) {
        int addr = IC_INIT_VALUE + code_count;
        if (!insert_label(label_table_head, inst->label, addr, 1, 0, 0)) {
            *error_found = 1;
            return 0;
        }
    }

    const OpcodeInfo *op = find_opcode(inst->opcode);
    if (!op) {
        *error_found = 1;
        return 0;
    }

    char src[128] = {0}, dst[128] = {0};
    int operands_found = split_operands(inst->operands, src, sizeof(src), dst, sizeof(dst));
    if (operands_found != op->num_operands) {
        *error_found = 1;
        return 0;
    }

    int src_mode = 0, dst_mode = 0;
    if (op->num_operands == 2) {
        src_mode = addr_mode(src);
        dst_mode = addr_mode(dst);
        if (src_mode < 0 || dst_mode < 0 || !op->valid_src_addr[src_mode] || !op->valid_dst_addr[dst_mode]) {
            *error_found = 1;
            return 0;
        }
    } else if (op->num_operands == 1) {
        dst_mode = addr_mode(src);
        if (dst_mode < 0 || !op->valid_dst_addr[dst_mode]) {
            *error_found = 1;
            return 0;
        }
        src_mode = 0; // per spec, bits for src are 0 in single-operand instructions
        // move single operand to dst variable for uniform handling below
        strncpy(dst, src, sizeof(dst)-1);
        dst[sizeof(dst)-1] = '\0';
        src[0] = '\0';
    } else {
        src_mode = 0; dst_mode = 0; // no operands
    }

    int emitted = 0;
    // First word
    code[code_count + emitted].value = pack_first_word(op->code, src_mode, dst_mode);
    code[code_count + emitted].are = 0; // absolute
    code[code_count + emitted].src_line = line_num;
    code[code_count + emitted].ext_name[0] = '\0';
    emitted++;

    // Handle extra words
    if (op->num_operands == 2) {
        // Register-register can share one word
        int both_regs = (src_mode == 3 && dst_mode == 3);
        if (both_regs) {
            int rs = regnum(src);
            int rd = regnum(dst);
            code[code_count + emitted].value = reg_pair_word(rs, rd);
            code[code_count + emitted].are = 0;
            code[code_count + emitted].ext_name[0] = '\0';
            emitted++;
        } else {
            // source operand extra words
            if (src_mode == 0) {
                int num = atoi(src + 1); // after '#'
                add_immediate_word(code, code_count + emitted, num);
                emitted++;
            } else if (src_mode == 1) {
                add_label_word(code, code_count + emitted, src);
                emitted++;
            } else if (src_mode == 2) {
                emitted += add_matrix_words(code, code_count + emitted, src);
            } else if (src_mode == 3) {
                int rs = regnum(src);
                code[code_count + emitted].value = reg_pair_word(rs, 0);
                code[code_count + emitted].are = 0;
                code[code_count + emitted].ext_name[0] = '\0';
                emitted++;
            }
            // dest operand extra words
            if (dst_mode == 0) {
                int num = atoi(dst + 1);
                add_immediate_word(code, code_count + emitted, num);
                emitted++;
            } else if (dst_mode == 1) {
                add_label_word(code, code_count + emitted, dst);
                emitted++;
            } else if (dst_mode == 2) {
                emitted += add_matrix_words(code, code_count + emitted, dst);
            } else if (dst_mode == 3) {
                int rd = regnum(dst);
                code[code_count + emitted].value = reg_pair_word(0, rd);
                code[code_count + emitted].are = 0;
                code[code_count + emitted].ext_name[0] = '\0';
                emitted++;
            }
        }
    } else if (op->num_operands == 1) {
        // Single operand acts like dest
        if (dst_mode == 0) {
            int num = atoi(dst + 1);
            add_immediate_word(code, code_count + emitted, num);
            emitted++;
        } else if (dst_mode == 1) {
            add_label_word(code, code_count + emitted, dst);
            emitted++;
        } else if (dst_mode == 2) {
            emitted += add_matrix_words(code, code_count + emitted, dst);
        } else if (dst_mode == 3) {
            int rd = regnum(dst);
            code[code_count + emitted].value = reg_pair_word(0, rd);
            code[code_count + emitted].are = 0;
            code[code_count + emitted].ext_name[0] = '\0';
            emitted++;
        }
    }

    return emitted;
}
