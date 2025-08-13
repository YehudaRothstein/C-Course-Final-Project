#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

void remove_extra_spaces_str(char *str) {
    
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


inst_parts parse_inst_line(char *line) {
    inst_parts inst;
    char *p;
    char *colon;
    char *after_colon = NULL;
    char *opcode_start;

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


void free_inst_parts(inst_parts *inst) {
    if (!inst) return;
    if (inst->label) { free(inst->label); inst->label = NULL; }
    if (inst->opcode) { free(inst->opcode); inst->opcode = NULL; }
    if (inst->operands) { free(inst->operands); inst->operands = NULL; }
    if (inst->src_operand) { free(inst->src_operand); inst->src_operand = NULL; }
    if (inst->dst_operand) { free(inst->dst_operand); inst->dst_operand = NULL; }
}

int is_instr(const char *opcode) {
    
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
