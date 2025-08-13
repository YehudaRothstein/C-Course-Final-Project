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
#include "data_conv.h"
#include "code_conversion.h"
#include "memory_map.h"
#include "memory_map_data.h"
#include "modular_helpers.h"
#include "data_directive.h"
#include "output_writer.h"
#include "data_word.h"
#include "memory_map_print.h"
#include "second-run.h"

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
    switch (error_code) {
        case 1: msg = "Label is empty or NULL"; break;
        case 2: msg = "Label is too long (max 30 chars)"; break;
        case 3: msg = "Label must start with a letter"; break;
        case 4: msg = "Label contains non-alphanumeric characters"; break;
        case 5: msg = "Label is a reserved word (opcode/instruction)"; break;
        case 6: msg = "Label is a register name (r0-r7)"; break;
        default: msg = "Unknown label error"; break;
    }
    printf("[ERROR] %s (file: %s, line: %d)\n", msg, file_name, line_num);
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


static void process_data_directives(DataDirective *data_directives, int data_directives_count, int data_base_addr, data_word **data_image_ptr, int *data_count_ptr, int *DC_ptr, LabelNode **label_table_head_ptr) {
    int DC = *DC_ptr;
    int data_count = *data_count_ptr;
    data_word *data_image = *data_image_ptr;
    LabelNode *label_table_head = *label_table_head_ptr;
    int i;
    printf("[DEBUG] process_data_directives: data_base_addr=%d, data_directives_count=%d\n", data_base_addr, data_directives_count);
    for (i = 0; i < data_directives_count; i++) {
        DataDirective *dd = &data_directives[i];
        int base_dc = DC;
        printf("[DEBUG] DataDirective %d: type=%s, label=%s, operands=%s, src_line=%d\n", i, dd->type, dd->label, dd->operands, dd->src_line);
        if (strcmp(dd->type, ".data") == 0) {
            if (dd->label[0]) {
                printf("[DEBUG] Adding data label: %s at addr %d\n", dd->label, data_base_addr + DC);
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC;
            }
            {
                const char *p = dd->operands;
                char numbuf[32];
                int data_idx = base_dc;
                while (*p) {
                    while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
                    if (!*p) break;
                    {
                        int j = 0;
                        if (*p == '+' || *p == '-') numbuf[j++] = *p++;
                        while (*p && isdigit((unsigned char)*p) && j < 30) numbuf[j++] = *p++;
                        numbuf[j] = '\0';
                        if (j == 0) break;
                        {
                            int val = atoi(numbuf);
                            printf("[DEBUG] Adding data value: %d at data_idx %d\n", val, data_idx);
                            data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                            data_image[data_count].value = (unsigned short)(val & 0x3FF);
                            data_image[data_count].src_line = dd->src_line;
                            memory_map_set(&data_memory_map, data_idx, (unsigned short)(val & 0x3FF));
                            data_count++;
                            DC++;
                            data_idx++;
                        }
                    }
                }
            }
        } else if (strcmp(dd->type, ".string") == 0) {
            if (dd->label[0]) {
                printf("[DEBUG] Adding string label: %s at addr %d\n", dd->label, data_base_addr + DC);
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC;
            }
            {
                const char *p = dd->operands;
                while (*p && *p != '"') p++;
                if (*p == '"') {
                    const char *start;
                    const char *end = NULL;
                    int data_idx = base_dc;
                    p++;
                    start = p;
                    while (*p) {
                        if (*p == '"') { end = p; break; }
                        p++;
                    }
                    if (end && end > start) {
                        const char *q;
                        for (q = start; q < end; q++) {
                            printf("[DEBUG] Adding string char: %c (%d) at data_idx %d\n", *q, (unsigned char)*q, data_idx);
                            data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                            data_image[data_count].value = (unsigned short)((unsigned char)*q & 0x3FF);
                            data_image[data_count].src_line = dd->src_line;
                            memory_map_set(&data_memory_map, data_idx, (unsigned short)((unsigned char)*q & 0x3FF));
                            data_count++;
                            DC++;
                            data_idx++;
                        }
                        printf("[DEBUG] Adding string terminator at data_idx %d\n", data_idx);
                        data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                        data_image[data_count].value = 0;
                        data_image[data_count].src_line = dd->src_line;
                        memory_map_set(&data_memory_map, data_idx, 0);
                        data_count++;
                        DC++;
                    }
                }
            }
        } else if (strcmp(dd->type, ".mat") == 0) {
            if (dd->label[0]) {
                printf("[DEBUG] Adding mat label: %s at addr %d\n", dd->label, data_base_addr + DC);
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC;
            }
            {
                const char *p = dd->operands;
                int rows = 0, cols = 0;
                int total;
                int mat_vals;
                char numbuf[32];
                int data_idx;
                if (p && *p == '[') {
                    p++;
                    rows = atoi(p);
                    p = strchr(p, ']');
                    if (p && *(p+1) == '[') {
                        p += 2;
                        cols = atoi(p);
                        p = strchr(p, ']');
                        if (p) p++;
                    }
                }
                while (p && (*p == ' ' || *p == '\t' || *p == ',')) p++;
                total = rows * cols;
                mat_vals = 0;
                data_idx = base_dc;
                while (mat_vals < total && p && *p) {
                    while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
                    if (!*p) break;
                    {
                        int j = 0;
                        if (*p == '+' || *p == '-') numbuf[j++] = *p++;
                        while (*p && isdigit((unsigned char)*p) && j < 30) numbuf[j++] = *p++;
                        numbuf[j] = '\0';
                        if (j == 0) break;
                        {
                            int val = atoi(numbuf);
                            printf("[DEBUG] Adding mat value: %d at data_idx %d\n", val, data_idx);
                            data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                            data_image[data_count].value = (unsigned short)(val & 0x3FF);
                            data_image[data_count].src_line = dd->src_line;
                            memory_map_set(&data_memory_map, data_idx, (unsigned short)(val & 0x3FF));
                            data_count++;
                            DC++;
                            mat_vals++;
                            data_idx++;
                        }
                    }
                }
                while (mat_vals < total) {
                    printf("[DEBUG] Adding mat zero at data_idx %d\n", data_idx);
                    data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                    data_image[data_count].value = 0;
                    data_image[data_count].src_line = dd->src_line;
                    memory_map_set(&data_memory_map, data_idx, 0);
                    data_count++;
                    DC++;
                    mat_vals++;
                    data_idx++;
                }
            }
        }
    }
    *data_image_ptr = data_image;
    *data_count_ptr = data_count;
    *DC_ptr = DC;
    *label_table_head_ptr = label_table_head;
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
        printf("[ERROR] Failed to open file: %s\n", file_name);
        handleError(1, file_name);
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
            printf("[ERROR] Line %d too long\n", line_num);
            handleError(1, file_name);
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
                printf("[LOG] Found label '%s' at line %d\n", inst.label, line_num);
                if (!legal_label_decl(inst.label, &error_code)) {
                    printf("[ERROR] Illegal label '%s' at line %d\n", inst.label, line_num);
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

