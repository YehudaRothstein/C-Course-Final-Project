/* C-Course Final Project - Assembler (authored by Yehuda) */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"
#include "code_conversion.h"
#include "label_table.h"
#include "emit/modular_helpers.h"
#include "../../error-handler/error-handler.h"

#ifndef IC_INIT_VALUE
#define IC_INIT_VALUE 100
#endif

static int addressingMode(const char *operand) {
    if (!operand || !*operand) return -1;
    if (operand[0] == '#') return 0;
    if (operand[0] == 'r' && operand[1] >= '0' && operand[1] <= '7' && operand[2] == '\0') return 3;
    if (strchr(operand, '[')) return 2;
    return 1;
}

static void trimString(char *s) {
    if (!s) return;
    {
        char *p = s;
        while (isspace((unsigned char)*p)) p++;
        if (p != s) memmove(s, p, strlen(p) + 1);
    }
    {
        size_t n = strlen(s);
        while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    }
}

static int splitOperands(const char *operands, char *source, size_t sourceSize, char *destination, size_t destinationSize) {
    const char *comma;
    size_t len1;
    if (!operands || !*operands) return 0;
    comma = strchr(operands, ',');
    if (!comma) {
        strncpy(source, operands, sourceSize - 1);
        source[sourceSize - 1] = '\0';
        trimString(source);
        return *source ? 1 : 0;
    }
    len1 = (size_t)(comma - operands);
    if (len1 >= sourceSize) len1 = sourceSize - 1;
    memcpy(source, operands, len1);
    source[len1] = '\0';
    trimString(source);
    strncpy(destination, comma + 1, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
    trimString(destination);
    return (*source ? 1 : 0) + (*destination ? 1 : 0);
}

static unsigned short packFirstWord(int opcode, int sourceMode, int destinationMode) {
    unsigned short w = 0;
    w |= ((unsigned short)(opcode & 0xF)) << 6;
    w |= ((unsigned short)(sourceMode & 0x3)) << 4;
    w |= ((unsigned short)(destinationMode & 0x3)) << 2;
    return w & 0x3FF;
}

int regnum(const char *reg) {
    if (reg && reg[0] == 'r' && reg[1] >= '0' && reg[1] <= '7' && reg[2] == '\0') return reg[1] - '0';
    return 0;
}

int mat_reg(const char *mat, int which) {
    const char *p = mat;
    int found = 0;
    while (*p) {
        if (*p == '[') {
            if (which == found) {
                p++;
                if (*p == 'r' && p[1] >= '0' && p[1] <= '7') return p[1] - '0';
            }
            found++;
        }
        p++;
    }
    return 0;
}

void mat_label(const char *mat, char *out) {
    const char *p = strchr(mat, '[');
    if (!p) {
        strcpy(out, mat);
        return;
    }
    {
        size_t len = (size_t)(p - mat);
        strncpy(out, mat, len);
        out[len] = '\0';
    }
}

static unsigned short packRegisterPairWord(int sourceRegister, int destinationRegister) {
    unsigned short w = 0;
    w |= ((unsigned short)(sourceRegister & 0xF)) << 6;
    w |= ((unsigned short)(destinationRegister & 0xF)) << 2;
    return w & 0x3FF;
}

static void writeLabelWord(code_conv_t *code, int index, const char *symbol) {
    code[index].value = 0;
    code[index].are = 1;
    if (symbol && *symbol) {
        strncpy(code[index].ext_name, symbol, sizeof(code[index].ext_name) - 1);
        code[index].ext_name[sizeof(code[index].ext_name) - 1] = '\0';
    } else {
        code[index].ext_name[0] = '\0';
    }
}

static void writeImmediateWord(code_conv_t *code, int index, int number) {
    unsigned short w = ((unsigned short)(number & 0xFF)) << 2;
    code[index].value = w & 0x3FF;
    code[index].are = 0;
    code[index].ext_name[0] = '\0';
}

static int writeMatrixWords(code_conv_t *code, int index, const char *operand) {
    char label[64];
    int rowRegister;
    int colRegister;
    mat_label(operand, label);
    writeLabelWord(code, index, label);
    rowRegister = mat_reg(operand, 0);
    colRegister = mat_reg(operand, 1);
    code[index + 1].value = packRegisterPairWord(rowRegister, colRegister);
    code[index + 1].are = 0;
    code[index + 1].ext_name[0] = '\0';
    return 2;
}

static int writeDestinationWords(code_conv_t *code, int index, const char *destination, int destinationMode) {
    int written = 0;
    if (destinationMode == 0) {
        int number = atoi(destination + 1);
        writeImmediateWord(code, index + written, number);
        written++;
    } else if (destinationMode == 1) {
        writeLabelWord(code, index + written, destination);
        written++;
    } else if (destinationMode == 2) {
        written += writeMatrixWords(code, index + written, destination);
    } else if (destinationMode == 3) {
        int destinationRegister = regnum(destination);
        code[index + written].value = packRegisterPairWord(0, destinationRegister);
        code[index + written].are = 0;
        code[index + written].ext_name[0] = '\0';
        written++;
    }
    return written;
}

static int writeSourceWords(code_conv_t *code, int index, const char *source, int sourceMode) {
    int written = 0;
    if (sourceMode == 0) {
        int number = atoi(source + 1);
        writeImmediateWord(code, index + written, number);
        written++;
    } else if (sourceMode == 1) {
        writeLabelWord(code, index + written, source);
        written++;
    } else if (sourceMode == 2) {
        written += writeMatrixWords(code, index + written, source);
    } else if (sourceMode == 3) {
        int sourceRegister = regnum(source);
        code[index + written].value = packRegisterPairWord(sourceRegister, 0);
        code[index + written].are = 0;
        code[index + written].ext_name[0] = '\0';
        written++;
    }
    return written;
}

static int writeTwoOperandsWords(code_conv_t *code, int index, const char *source, int sourceMode, const char *destination, int destinationMode) {
    if (sourceMode == 3 && destinationMode == 3) {
        int sourceRegister = regnum(source);
        int destinationRegister = regnum(destination);
        code[index].value = packRegisterPairWord(sourceRegister, destinationRegister);
        code[index].are = 0;
        code[index].ext_name[0] = '\0';
        return 1;
    }
    {
        int used = 0;
        used += writeSourceWords(code, index + used, source, sourceMode);
        used += writeDestinationWords(code, index + used, destination, destinationMode);
        return used;
    }
}

static int parseOperandsToBuffers(const char *operands, char *source, size_t sourceSize, char *destination, size_t destinationSize) {
    return splitOperands(operands, source, sourceSize, destination, destinationSize);
}

static void getTwoOperandModes(const char *source, const char *destination, int *sourceMode, int *destinationMode) {
    *sourceMode = addressingMode(source);
    *destinationMode = addressingMode(destination);
}

static void getOneOperandMode(const char *source, int *destinationMode, char *destination, size_t destinationSize) {
    *destinationMode = addressingMode(source);
    strncpy(destination, source, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
}

static int areOperandModesValid(const OpCodeData *op, int sourceMode, int destinationMode) {
    if (op->operand_count == 2) {
        return (sourceMode >= 0 && destinationMode >= 0 && op->valid_src_addr[sourceMode] && op->valid_dst_addr[destinationMode]);
    }
    if (op->operand_count == 1) {
        return (destinationMode >= 0 && op->valid_dst_addr[destinationMode]);
    }
    /* no operands */
    return 1;
}

static int prepareOperands(const OpCodeData *op, const char *operands, char *source, size_t sourceSize, char *destination, size_t destinationSize, int *sourceMode, int *destinationMode, const char *fileName, int lineNumber, int *errorFound) {
    int found;
    char buf[128];
    int err;

    found = parseOperandsToBuffers(operands, source, sourceSize, destination, destinationSize);
    if (found != op->operand_count) {
        /* ANSI C89: use sprintf instead of snprintf */
        sprintf(buf, "expected %d got %d", op->operand_count, found);
        error_report_ex(ERR_SEV_ERROR, ERR_OPERAND_COUNT_MISMATCH, fileName, lineNumber, buf);
        *errorFound = 1;
        return 0;
    }
    if (op->operand_count == 2) {
        getTwoOperandModes(source, destination, sourceMode, destinationMode);
    } else if (op->operand_count == 1) {
        getOneOperandMode(source, destinationMode, destination, destinationSize);
        *sourceMode = 0;
        source[0] = '\0';
    } else {
        *sourceMode = 0;
        *destinationMode = 0;
    }
    if (!areOperandModesValid(op, *sourceMode, *destinationMode)) {
        *errorFound = 1;
        err = (op->operand_count == 2 && (!op->valid_src_addr[*sourceMode] || *sourceMode < 0)) ? ERR_ADDR_MODE_INVALID_SRC : ERR_ADDR_MODE_INVALID_DST;
        error_report_ex(ERR_SEV_ERROR, err, fileName, lineNumber, NULL);
        return 0;
    }
    return 1;
}

static int handleLabelDefinition(InstParts *inst, int codeCount, LabelNode **labels, int *errorFound) {
    if (inst->label && *inst->label) {
        int address = IC_INIT_VALUE + codeCount;
        if (!insert_label(labels, inst->label, address, 1, 0, 0)) { *errorFound = 1; return 0; }
    }
    return 1;
}

static void writeFirstWord(code_conv_t *code, int index, const OpCodeData *op, int sourceMode, int destinationMode, int lineNumber) {
    code[index].value = packFirstWord(op->code, sourceMode, destinationMode);
    code[index].are = 0;
    code[index].source_line_num = lineNumber;
    code[index].ext_name[0] = '\0';
}

int assembleInstruction(InstParts *inst, code_conv_t *code, int codeCount, int lineNumber, LabelNode **labelTableHead, int *errorFound, const char *fileName) {
    int written = 0;
    char source[128];
    char destination[128];
    int sourceMode = 0, destinationMode = 0;
    const OpCodeData *op;
    char buf[128];
    (void)fileName;
    if (!inst || !inst->opcode) return 0;
    if (!handleLabelDefinition(inst, codeCount, labelTableHead, errorFound)) return 0;
    op = findOpcodeByName(inst->opcode);
    if (!op) {
        *errorFound = 1;
        /* ANSI C89: use sprintf instead of snprintf */
        sprintf(buf, "opcode '%s'", inst->opcode);
        error_report_ex(ERR_SEV_ERROR, ERR_OPCODE_UNKNOWN, fileName, lineNumber, buf);
        return 0;
    }
    source[0] = '\0';
    destination[0] = '\0';
    if (!prepareOperands(op, inst->operands, source, sizeof(source), destination, sizeof(destination), &sourceMode, &destinationMode, fileName, lineNumber, errorFound)) return 0;
    writeFirstWord(code, codeCount + written, op, sourceMode, destinationMode, lineNumber);
    written++;
    if (op->operand_count == 2) written += writeTwoOperandsWords(code, codeCount + written, source, sourceMode, destination, destinationMode);
    else if (op->operand_count == 1) written += writeDestinationWords(code, codeCount + written, destination, destinationMode);
    return written;
}

int emit_instruction(InstParts *inst, code_conv_t *code, int code_count, int line_num, LabelNode **label_table_head, int *error_found, const char *file_name) {
    return assembleInstruction(inst, code, code_count, line_num, label_table_head, error_found, file_name);
}
