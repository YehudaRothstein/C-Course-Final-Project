// Minimal stubs for missing utility functions
#include "utils.h"
#include <string.h>
#include <stdlib.h>

void remove_extra_spaces_str(char *str) {
    // Remove extra spaces (stub)
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

void remove_spaces_next_to_comma(char *str) {
    // Remove spaces next to commas (stub)
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


#include <ctype.h>
#include <stdio.h>

// Helper: duplicate a substring
static char *substrdup(const char *start, size_t len) {
    char *s = (char *)malloc(len + 1);
    if (!s) return NULL;
    memcpy(s, start, len);
    s[len] = '\0';
    return s;
}

// Parse a line into label, opcode, operands
inst_parts parse_inst_line(char *line) {
    inst_parts inst = {0};
    char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '\0' || *p == ';') return inst; // empty or comment

    // Find label (if any)
    char *colon = strchr(p, ':');
    char *after_colon = NULL;
    if (colon && (colon > p)) {
        // Label exists
        const char *label_start = p;
        size_t label_len = colon - p;
        inst.label = substrdup(label_start, label_len);
        after_colon = colon + 1;
        p = after_colon;
        while (isspace((unsigned char)*p)) p++;
    }

    // Find opcode
    char *opcode_start = p;
    while (*p && !isspace((unsigned char)*p)) p++;
    if (p > opcode_start) {
        inst.opcode = substrdup(opcode_start, p - opcode_start);
    } else {
        // No opcode found
        return inst;
    }
    while (isspace((unsigned char)*p)) p++;

    // The rest is operands (if any)
    if (*p) {
        inst.operands = strdup(p);
        // Remove trailing whitespace
        size_t len = strlen(inst.operands);
        while (len > 0 && isspace((unsigned char)inst.operands[len-1])) {
            inst.operands[--len] = '\0';
        }
    }

    return inst;
}

// Free all dynamically allocated fields in inst_parts
void free_inst_parts(inst_parts *inst) {
    if (!inst) return;
    if (inst->label) { free(inst->label); inst->label = NULL; }
    if (inst->opcode) { free(inst->opcode); inst->opcode = NULL; }
    if (inst->operands) { free(inst->operands); inst->operands = NULL; }
    if (inst->src_operand) { free(inst->src_operand); inst->src_operand = NULL; }
    if (inst->dst_operand) { free(inst->dst_operand); inst->dst_operand = NULL; }
}

int is_instr(const char *opcode) {
    // Minimal stub: checks for dot at start
    return opcode && opcode[0] == '.';
}
#include <ctype.h>
#include "utils.h"

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
