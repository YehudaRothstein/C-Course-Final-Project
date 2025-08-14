#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"
#include "error-handler.h"
#include "label_table.h"
#include "other_table.h"
#include "../first-pass/data/data_conv.h"
#include "code_conversion.h"
#include "memory_map.h"
#include "memory_map_data.h"
#include "../first-pass/emit/modular_helpers.h"
#include "data_directive.h"
#include "../first-pass/output/output_writer.h"
#include "data_word.h"
#include "../first-pass/print/memory_map_print.h"
#include "second-run/second-run.h"
#include "../first-pass/data/process_data_directives.h"

#ifndef IC_INIT_VALUE
#define IC_INIT_VALUE 100
#endif


int print_symbol_table(LabelNode *head) {
    LabelNode *curr = head;
    while (curr) {
        const char *type = curr->is_code ? "code" :
                           (curr->is_data ? "data" :
                           (curr->is_extern ? "extern" : "?"));
        printf("%s %d %s\n", curr->name, curr->address, type);
        curr = curr->next;
    }
    return 0;
}


int legal_label_decl(const char *label, int *error_code) {
    int i;
    int len;
    if (!label || !label[0]) { if (error_code) *error_code = 1; return 0; }
    len = strlen(label);
    if (len > 30) { if (error_code) *error_code = 2; return 0; }
    if (!isalpha((unsigned char)label[0])) { if (error_code) *error_code = 3; return 0; }
    for (i = 1; i < len; i++) {
        if (!isalnum((unsigned char)label[i])) { if (error_code) *error_code = 4; return 0; }
    }
    if (find_opcode(label) != NULL || is_instr(label)) { if (error_code) *error_code = 5; return 0; }
    if (len == 2 && label[0] == 'r' && label[1] >= '0' && label[1] <= '7') { if (error_code) *error_code = 6; return 0; }
    if (error_code) *error_code = 0;
    return 1;
}

void print_external_error(int error_code, const char *file_name, int line_num) {
    const char *msg = NULL;
    ErrorCode mapped = ERR_NONE;
    switch (error_code) {
        case 1: msg = "Label is empty or NULL"; mapped = ERR_LABEL_EMPTY; break;
        case 2: msg = "Label is too long (max 30 chars)"; mapped = ERR_LABEL_TOO_LONG; break;
        case 3: msg = "Label must start with a letter"; mapped = ERR_LABEL_NOT_START_ALPHA; break;
        case 4: msg = "Label contains non-alphanumeric characters"; mapped = ERR_LABEL_NON_ALNUM; break;
        case 5: msg = "Label is a reserved word (opcode/instruction)"; mapped = ERR_LABEL_RESERVED_WORD; break;
        case 6: msg = "Label is a register name (r0-r7)"; mapped = ERR_LABEL_IS_REGISTER; break;
        default: msg = "Unknown label error"; mapped = ERR_SYMBOL_TABLE_INCONSISTENT; break;
    }
    error_report_ex(ERR_SEV_ERROR, mapped, file_name, line_num, "%s", msg);
}

int process_data(const char *operands, data_conv **data) {
    int count = 0;
    const char *p;
    char numbuf[32];
    int i;
    if (!operands) return 0;
    p = operands;
    while (*p) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (!*p) break;
        i = 0;
        if (*p == '+' || *p == '-') numbuf[i++] = *p++;
        while (*p && isdigit((unsigned char)*p) && i < 30) numbuf[i++] = *p++;
        numbuf[i] = '\0';
        if (i == 0) break;
        {
            int val = atoi(numbuf);
            if (data && *data) { (*data)[count].value = val; }
            count++;
        }
    }
    return count;
}

int process_string(const char *operands, data_conv **data) {
    const char *start;
    const char *end;
    const char *p;
    int count = 0;
    if (!operands) return 0;
    start = strchr(operands, '"');
    if (!start) return 0;
    start++;
    end = strchr(start, '"');
    if (!end) return 0;
    for (p = start; p < end; p++) {
        if (data && *data) { (*data)[count].value = (unsigned char)*p; }
        count++;
    }
    if (data && *data) { (*data)[count].value = 0; }
    count++;
    return count;
}

void add_to_other_table(other_table **table, int *count, const char *operand) {
    const char *p;
    const char *end;
    int len;
    other_table *new_table;
    if (!operand || !table || !count) return;
    p = operand;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) return;
    end = p;
    while (*end && !isspace((unsigned char)*end) && *end != ',') end++;
    len = (int)(end - p);
    if (len <= 0) return;
    if (len > 31) len = 31;
    new_table = realloc(*table, (*count + 1) * sizeof(other_table));
    if (!new_table) return;
    *table = new_table;
#ifdef OTHER_TABLE_LABEL_FIELD
    strncpy((*table)[*count].OTHER_TABLE_LABEL_FIELD, p, len);
    (*table)[*count].OTHER_TABLE_LABEL_FIELD[len] = '\0';
#else
    strncpy((*table)[*count].name, p, len);
    (*table)[*count].name[len] = '\0';
#endif
    (*count)++;
}



static void get_basefile(const char *file_name, char *basefile, size_t basefile_size) {
    const char *slash = strrchr(file_name, '/');
    const char *base = slash ? slash + 1 : file_name;
    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    if (len >= basefile_size) len = basefile_size - 1;
    strncpy(basefile, base, len);
    basefile[len] = '\0';
}


int exe_first_pass(char *file_name) {
    char line_buf[MAX_LINE_LENGTH];
    int DC = 0;
    int error_found = 0;
    LabelNode *label_table_head = NULL;
    other_table *externs = NULL, *entries = NULL;
    int externs_count = 0, entries_count = 0;
    code_conv code[1024];
    int i;
    int code_count = 0;
    data_word *data_image = NULL;
    int data_count = 0;
    DataDirective *data_directives = NULL;
    int data_directives_count = 0;
    FILE *fp;
    int line_num = 0;
    int data_base_addr;
    char basefile[256];

    for (i = 0; i < 1024; ++i) code[i].ext_name[0] = '\0';
    memory_map_init(&data_memory_map);

    printf("[LOG] Opening file: %s\n", file_name);
    fp = fopen(file_name, "r");
    if (!fp) {
        error_report(ERR_IO_INPUT_OPEN_FAIL, file_name, 0, file_name);
        return 1;
    }

    while (fgets(line_buf, MAX_LINE_LENGTH, fp)) {
        line_num++;
        printf("[LOG] Read line %d: %s", line_num, line_buf);
        if (line_buf[0] == '\n' || line_buf[0] == ';') {
            printf("[LOG] Skipping empty/comment line %d\n", line_num);
            continue;
        }
        printf("[LOG] Cleaning up line %d\n", line_num);
        remove_extra_spaces_str(line_buf);
        remove_spaces_next_to_comma(line_buf);
        if (strlen(line_buf) > MAX_LINE_LENGTH - 1) {
            error_report_ex(ERR_SEV_ERROR, ERR_LINE_TOO_LONG, file_name, line_num, NULL);
            error_found = 1;
            continue;
        }
        printf("[LOG] Parsing line %d\n", line_num);
        {
            inst_parts inst = parse_inst_line(line_buf);
            printf("[LOG] Parsed line %d: label='%s', opcode='%s', operands='%s'\n", line_num,
                inst.label ? inst.label : "(null)",
                inst.opcode ? inst.opcode : "(null)",
                inst.operands ? inst.operands : "(null)");
            if (!inst.opcode) {
                printf("[LOG] No opcode at line %d (empty or whitespace line). Skipping.\n", line_num);
                free_inst_parts(&inst);
                continue;
            }
            if (inst.label) {
                int error_code = 0;
                if (!legal_label_decl(inst.label, &error_code)) {
                    error_report_ex(ERR_SEV_ERROR, ERR_LABEL_REDEFINED, file_name, line_num, "Illegal label '%s'", inst.label);
                    print_external_error(error_code, file_name, line_num);
                    error_found = 1;
                    free_inst_parts(&inst);
                    continue;
                }
            }
            if (is_instr(inst.opcode)) {
                if (strcmp(inst.opcode, ".data") == 0 || strcmp(inst.opcode, ".string") == 0 || strcmp(inst.opcode, ".mat") == 0) {
                    data_directives = realloc(data_directives, (data_directives_count + 1) * sizeof(DataDirective));
                    strncpy(data_directives[data_directives_count].type, inst.opcode, 7);
                    data_directives[data_directives_count].type[7] = '\0';
                    if (inst.label) {
                        strncpy(data_directives[data_directives_count].label, inst.label, 31);
                        data_directives[data_directives_count].label[31] = '\0';
                    } else {
                        data_directives[data_directives_count].label[0] = '\0';
                    }
                    if (inst.operands) {
                        strncpy(data_directives[data_directives_count].operands, inst.operands, MAX_LINE_LENGTH-1);
                        data_directives[data_directives_count].operands[MAX_LINE_LENGTH-1] = '\0';
                    } else {
                        data_directives[data_directives_count].operands[0] = '\0';
                    }
                    data_directives[data_directives_count].src_line = line_num;
                    data_directives_count++;
                } else if (strcmp(inst.opcode, ".extern") == 0) {
                    add_to_other_table(&externs, &externs_count, inst.operands);
                } else if (strcmp(inst.opcode, ".entry") == 0) {
                    add_to_other_table(&entries, &entries_count, inst.operands);
                }
            }
            else {
                
                {
                    int emitted = emit_instruction(&inst, code, code_count, line_num, &label_table_head, &error_found, file_name);
                    if (emitted > 0) {
                        code_count += emitted;
                    }
                }
            }
            free_inst_parts(&inst);
        }
    }
    fclose(fp);

    
    DC = 0;
    data_count = 0;
    if (data_image) { free(data_image); data_image = NULL; }
    data_base_addr = IC_INIT_VALUE + code_count;
    process_data_directives(data_directives, data_directives_count, data_base_addr, &data_image, &data_count, &DC, &label_table_head);
    if (data_directives) free(data_directives);

    update_data_labels_address(label_table_head);
    if (!check_duplicate_labels(label_table_head)) {
        error_found = 1;
    }

    /* Proceed to outputs only if no errors collected */
    if (error_found || error_get_error_count() > 0) {
        error_report_ex(ERR_SEV_ERROR, ERR_PASS1_FAILED, file_name, 0, NULL);
        free_label_list(label_table_head);
        if (entries) free(entries);
        if (externs) free(externs);
        if (data_image) free(data_image);
        return 1;
    }

    
    basefile[0] = '\0';
    get_basefile(file_name, basefile, sizeof(basefile));

    exe_second_pass(code, code_count, label_table_head, entries, entries_count, externs, externs_count, basefile);
    write_code_file(basefile, code, code_count, data_image, data_count);
    print_memory_map(code_count, code, data_count, data_image, label_table_head);
    print_symbol_table(label_table_head);

    free_label_list(label_table_head);
    if (entries) free(entries);
    if (externs) free(externs);
    if (data_image) free(data_image);
    return error_found;
}

