/* C-Course Final Project - Assembler (authored by Yehuda) */
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "../error-handler/error-handler.h"

void removeExtraSpacesStr(char *str) {
    char *src = str, *dst = str;
    int space = 0;
    while (*src) {
        if (*src == ' ' || *src == '\t') {
            if (!space) *dst++ = ' ';
            space = 1;
        } else {
            *dst++ = *src;
            space = 0;
        }
        src++;
    }
    *dst = '\0';
}

void removeSpacesNextToComma(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if ((*src == ' ' || *src == '\t') && (src > str && (*(src-1) == ',' || *(src+1) == ','))) {
            src++;
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

static char *substrdup(const char *start, size_t len) {
    char *s = (char *)malloc(len + 1);
    if (!s) return NULL;
    memcpy(s, start, len);
    s[len] = '\0';
    return s;
}

InstParts parseInstLine(char *line) {
    InstParts inst;
    char *p;
    char *colon;
    char *after_colon = NULL;
    char *opcode_start;
    char *token_end; /* end of the first non-whitespace token */

    inst.label = NULL;
    inst.opcode = NULL;
    inst.operands = NULL;
    inst.src_operand = NULL;
    inst.dst_operand = NULL;

    p = line;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '\0' || *p == ';') return inst;

    colon = strchr(p, ':');
    if (colon && (colon > p)) {
        const char *label_start = p;
        size_t label_len = (size_t)(colon - p);
        inst.label = substrdup(label_start, label_len);
        after_colon = colon + 1;
        p = after_colon;
        while (isspace((unsigned char)*p)) p++;
    }

    opcode_start = p;
    while (*p && !isspace((unsigned char)*p)) p++;
    token_end = p;

    /* Special-case adjacency syntax for .mat: allow ".mat[rows][cols]" */
    if (token_end > opcode_start && (size_t)(token_end - opcode_start) >= 5 && strncmp(opcode_start, ".mat[", 5) == 0) {
        const char *br = opcode_start + 4; /* points to '[' */
        size_t op_len;
        /* opcode is exactly ".mat" */
        inst.opcode = substrdup(".mat", 4);
        /* operands are from '[' until end of line (includes any following initializers) */
        inst.operands = substrdup(br, strlen(br));
        /* Trim trailing whitespace from operands */
        if (inst.operands) {
            op_len = strlen(inst.operands);
            while (op_len > 0 && isspace((unsigned char)inst.operands[op_len - 1])) {
                inst.operands[--op_len] = '\0';
            }
        }
        return inst;
    }

    if (p > opcode_start) {
        inst.opcode = substrdup(opcode_start, (size_t)(p - opcode_start));
    } else {
        return inst;
    }
    while (isspace((unsigned char)*p)) p++;

    if (*p) {
        size_t len;
        inst.operands = substrdup(p, strlen(p));
        len = strlen(inst.operands);
        while (len > 0 && isspace((unsigned char)inst.operands[len-1])) {
            inst.operands[--len] = '\0';
        }
    }

    return inst;
}

void freeInstParts(InstParts *inst) {
    if (!inst) return;
    if (inst->label) { free(inst->label); inst->label = NULL; }
    if (inst->opcode) { free(inst->opcode); inst->opcode = NULL; }
    if (inst->operands) { free(inst->operands); inst->operands = NULL; }
    if (inst->src_operand) { free(inst->src_operand); inst->src_operand = NULL; }
    if (inst->dst_operand) { free(inst->dst_operand); inst->dst_operand = NULL; }
}

int isInstr(const char *opcode) {
    return opcode && opcode[0] == '.';
}

char* ltrim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    return str;
}

int startsWithIgnoreCase(const char* str, const char* prefix) {
    while (*prefix && *str && tolower((unsigned char)*str) == tolower((unsigned char)*prefix)) {
        str++;
        prefix++;
    }
    return *prefix == '\0';
}

/* Minimal label validation for .extern/.entry and labels before colon */
int legal_label_decl(const char *label, int *error_code) {
    const char *p;
    int len;
    if (error_code) *error_code = 0;
    if (!label || !*label) { if (error_code) *error_code = ERR_LABEL_EMPTY; return 0; }
    if (!isalpha((unsigned char)label[0])) { if (error_code) *error_code = ERR_LABEL_NOT_START_ALPHA; return 0; }
    len = (int)strlen(label);
    if (len > 30) { if (error_code) *error_code = ERR_LABEL_TOO_LONG; return 0; }
    p = label;
    while (*p) {
        if (!isalnum((unsigned char)*p)) { if (error_code) *error_code = ERR_LABEL_NON_ALNUM; return 0; }
        p++;
    }
    /* Disallow register names r0..r7 and opcodes names as labels would be ideal; keep minimal here */
    return 1;
}

void print_external_error(int error_code, const char *file, int line) {
    /* Map a few common error codes through error_report_ex for consistency */
    switch (error_code) {
        case ERR_LABEL_EMPTY:
        case ERR_LABEL_NOT_START_ALPHA:
        case ERR_LABEL_TOO_LONG:
        case ERR_LABEL_NON_ALNUM:
            error_report_ex(ERR_SEV_ERROR, (ErrorCode)error_code, file, line, NULL);
            break;
        default:
            error_report_ex(ERR_SEV_ERROR, ERR_LABEL_REDEFINED, file, line, NULL);
            break;
    }
}

void get_basefile(const char *path, char *out_base, size_t out_size) {
    const char *slash = NULL;
    const char *back = NULL;
    const char *base;
    const char *dot;
    size_t len;
    if (!path || !out_base || out_size == 0) return;
    slash = strrchr(path, '/');
    back = strrchr(path, '\\');
    base = (slash && back) ? (slash > back ? slash + 1 : back + 1)
                           : (slash ? slash + 1 : (back ? back + 1 : path));
    dot = strrchr(base, '.');
    len = dot ? (size_t)(dot - base) : strlen(base);
    if (len >= out_size) len = out_size - 1;
    memcpy(out_base, base, len);
    out_base[len] = '\0';
}
