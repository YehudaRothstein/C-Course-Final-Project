/* C-Course Final Project - Assembler (authored by Yehuda) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "first-run.h"
#include "../utils/utils.h"
#include "../error-handler/error-handler.h"
#include "../structures/label_table.h"
#include "../structures/other_table.h"
#include "../code_conversion/code_conversion.h"
#include "../first-pass/emit/modular_helpers.h"
#include "../first-pass/data/process_data_directives.h"
#include "../first-pass/output/output_writer.h"
#include "../first-pass/print/memory_map_print.h"
#include "../second-run/second-run.h"
#include "../memory_map/memory_map.h"

int print_symbol_table(LabelNode *head) {
    LabelNode *curr = head;
    int count = 0;
    while (curr) { printf("%s %d %d %d %d\n", curr->name, curr->address, curr->is_code, curr->is_data, curr->is_extern); curr = curr->next; count++; }
    return count;
}

int legal_label_decl(const char *label, int *error_code) {
    size_t len; const char *p;
    if (!label || !*label) { if (error_code) *error_code = 1; return 0; }
    if (!isalpha((unsigned char)label[0])) { if (error_code) *error_code = 2; return 0; }
    len = strlen(label); if (len > 30) { if (error_code) *error_code = 3; return 0; }
    p = label; while (*p) { if (!isalnum((unsigned char)*p)) { if (error_code) *error_code = 4; return 0; } p++; }
    if (findOpcodeByName(label) != NULL || isInstr(label)) { if (error_code) *error_code = 5; return 0; }
    return 1;
}

void print_external_error(int error_code, const char *file_name, int line_num) {
    ErrorCode mapped = ERR_LABEL_NOT_START_ALPHA;
    const char *msg = "";
    switch (error_code) {
        case 1: mapped = ERR_LABEL_EMPTY; msg = "Empty label name"; break;
        case 2: mapped = ERR_LABEL_NOT_START_ALPHA; msg = "Label must start with a letter"; break;
        case 3: mapped = ERR_LABEL_TOO_LONG; msg = "Label too long"; break;
        case 4: mapped = ERR_LABEL_NON_ALNUM; msg = "Label has invalid character"; break;
        case 5: mapped = ERR_LABEL_RESERVED_WORD; msg = "Label collides with reserved word"; break;
        default: mapped = ERR_LABEL_NOT_START_ALPHA; msg = "Label error"; break;
    }
    error_report_ex(ERR_SEV_ERROR, mapped, file_name, line_num, msg);
}

int process_data(const char *operands, data_conv_t **data) {
    const char *p = operands; int count = 0; int cap = 8; int *arr = (int*)malloc(sizeof(int)*cap); char *end;
    if (!arr) return 0;
    while (p && *p) {
        long v = strtol(p, &end, 10);
        if (p == end) break;
        if (count >= cap) { cap *= 2; arr = (int*)realloc(arr, sizeof(int)*cap); if (!arr) return 0; }
        arr[count++] = (int)v;
        p = strchr(end, ','); if (p) p++; else break;
    }
    *data = (data_conv_t*)arr;
    return count;
}

int process_string(const char *operands, data_conv_t **data) {
    const char *q; int count = 0; int cap = 16; unsigned char *buf = (unsigned char*)malloc(cap);
    if (!buf) return 0;
    q = operands; if (*q == '"') q++;
    while (*q && *q != '"') { if (count >= cap) { cap *= 2; buf = (unsigned char*)realloc(buf, cap); if (!buf) return 0; } buf[count++] = (unsigned char)*q++; }
    if (count >= cap) { cap++; buf = (unsigned char*)realloc(buf, cap); if (!buf) return 0; }
    buf[count++] = 0;
    *data = (data_conv_t*)buf;
    return count;
}

void add_to_other_table(other_table **table, int *count, const char *operand) {
    *table = (other_table*)realloc(*table, sizeof(other_table) * (*count + 1));
    if (!*table) return;
    strncpy((*table)[*count].name, operand, sizeof((*table)[*count].name)-1);
    (*table)[*count].name[sizeof((*table)[*count].name)-1] = '\0';
    (*count)++;
}

static void get_basefile(const char *file_name, char *basefile, size_t basefile_size) {
    const char *slash = strrchr(file_name, '/'); const char *back = strrchr(file_name, '\\'); const char *sep = slash ? slash : back; const char *name = sep ? sep + 1 : file_name; const char *dot = strrchr(name, '.'); size_t n = dot ? (size_t)(dot - name) : strlen(name);
    if (n >= basefile_size) {
        n = basefile_size - 1;
    }
    memcpy(basefile, name, n);
    basefile[n] = '\0';
}

int exe_first_pass(char *file_name) {
    FILE *fp; char line_buf[256]; int line_num = 0; int error_found = 0; int DC = 0; code_conv_t *code = NULL; int code_count = 0; int code_cap = 0; data_word *data_image = NULL; int data_count = 0; LabelNode *label_table_head = NULL; int entries_count = 0, externs_count = 0; other_table *entries = NULL, *externs = NULL; MemoryMap data_memory_map; int data_base_addr = 100;
    DataDirective *dataDirectives = NULL; int dataDirectivesCount = 0; int dataDirectivesCap = 0;
    memory_map_init(&data_memory_map);
    fp = fopen(file_name, "r"); if (!fp) { error_report(ERR_IO_INPUT_OPEN_FAIL, file_name, 0, file_name); return 1; }
    while (fgets(line_buf, sizeof(line_buf), fp)) {
        int line_len = (int)strlen(line_buf); line_num++;
        if (line_len >= 1 && (line_buf[line_len-1] == '\n' || line_buf[line_len-1] == '\r')) line_buf[line_len-1] = '\0';
        removeExtraSpacesStr(line_buf);
        removeSpacesNextToComma(line_buf);
        if ((int)strlen(line_buf) > 80) {
            error_report_ex(ERR_SEV_ERROR, ERR_LINE_TOO_LONG, file_name, line_num, NULL);
            continue;
        }
        if (line_buf[0] == '\0' || line_buf[0] == ';') continue;
        {
            InstParts inst = parseInstLine(line_buf);
            if (!inst.opcode) {
                freeInstParts(&inst);
                continue;
            }
            if (inst.label && *inst.label) {
                int error_code = 0;
                if (!legal_label_decl(inst.label, &error_code)) {
                    char details[100];
                    sprintf(details, "Illegal label '%s'", inst.label);
                    error_report_ex(ERR_SEV_ERROR, ERR_LABEL_REDEFINED, file_name, line_num, details);
                    print_external_error(error_code, file_name, line_num);
                    freeInstParts(&inst);
                    continue;
                }
            }
            if (isInstr(inst.opcode)) {
                if (strcmp(inst.opcode, ".extern") == 0) {
                    add_to_other_table(&externs, &externs_count, inst.operands);
                    /* Also register extern in label table so second pass can detect it */
                    if (inst.operands && *inst.operands) {
                        int lerr = 0;
                        if (!legal_label_decl(inst.operands, &lerr)) {
                            print_external_error(lerr, file_name, line_num);
                            error_found = 1;
                        } else {
                            LabelNode *existing = find_label(label_table_head, inst.operands);
                            if (existing) {
                                if (!existing->is_extern) {
                                    error_report_ex(ERR_SEV_ERROR, ERR_EXTERN_LOCAL_CONFLICT, file_name, line_num, inst.operands);
                                    error_found = 1;
                                } else {
                                    error_report_ex(ERR_SEV_WARNING, ERR_EXTERN_DUPLICATE, file_name, line_num, inst.operands);
                                }
                            } else {
                                insert_label(&label_table_head, inst.operands, 0, 0, 0, 1);
                            }
                        }
                    }
                } else if (strcmp(inst.opcode, ".entry") == 0) {
                    add_to_other_table(&entries, &entries_count, inst.operands);
                } else if (strcmp(inst.opcode, ".data") == 0 || strcmp(inst.opcode, ".string") == 0 || strcmp(inst.opcode, ".mat") == 0) {
                    /* Enforce adjacency for .mat: must be ".mat[" (allow spaces inside brackets) */
                    if (strcmp(inst.opcode, ".mat") == 0) {
                        if (strstr(line_buf, ".mat[") == NULL) {
                            error_report_ex(ERR_SEV_ERROR, ERR_MAT_SIZE_INVALID, file_name, line_num, "'.mat' must be followed immediately by '['");
                            freeInstParts(&inst);
                            continue;
                        }
                    }
                    if (dataDirectivesCount >= dataDirectivesCap) { int newCap = dataDirectivesCap ? dataDirectivesCap * 2 : 16; DataDirective *tmp = (DataDirective*)realloc(dataDirectives, sizeof(DataDirective) * newCap); if (!tmp) { freeInstParts(&inst); fclose(fp); free(code); free(data_image); free(entries); free(externs); free(dataDirectives); return 1; } dataDirectives = tmp; dataDirectivesCap = newCap; }
                    {
                        DataDirective *dd = &dataDirectives[dataDirectivesCount++];
                        memset(dd, 0, sizeof(*dd));
                        strncpy(dd->type, inst.opcode, sizeof(dd->type) - 1);
                        if (inst.label) strncpy(dd->label, inst.label, sizeof(dd->label) - 1);
                        if (inst.operands) strncpy(dd->operands, inst.operands, sizeof(dd->operands) - 1);
                        dd->src_line = line_num;
                    }
                }
            } else {
                if (code_count >= code_cap) { code_cap = code_cap ? code_cap * 2 : 64; code = (code_conv_t*)realloc(code, sizeof(code_conv_t) * code_cap); if (!code) { fclose(fp); free(dataDirectives); return 1; } }
                {
                    int emitted = emit_instruction(&inst, code, code_count, line_num, &label_table_head, &error_found, file_name);
                    if (emitted < 0) { error_found = 1; }
                    else { code_count += emitted; }
                }
            }
            freeInstParts(&inst);
        }
    }
    fclose(fp);
    if (dataDirectivesCount > 0) {
        data_base_addr = 100 + code_count;

        /* Compute required number of data words by scanning directives */
        {
            int required_words = 0;
            int idx;
            for (idx = 0; idx < dataDirectivesCount; idx++) {
                DataDirective *dd = &dataDirectives[idx];
                if (strcmp(dd->type, ".data") == 0) {
                    int *tmp = NULL;
                    int cnt = process_data(dd->operands, (data_conv_t**)&tmp);
                    if (cnt > 0) {
                        required_words += cnt;
                        free(tmp);
                    }
                } else if (strcmp(dd->type, ".string") == 0) {
                    unsigned char *tmp = NULL;
                    int cnt = process_string(dd->operands, (data_conv_t**)&tmp);
                    if (cnt > 0) {
                        required_words += cnt; /* includes terminating 0 */
                        free(tmp);
                    }
                } else if (strcmp(dd->type, ".mat") == 0) {
                    /* Parse matrix dimensions of form [rows][cols] at start of operands */
                    const char *p = dd->operands;
                    int rows = 0, cols = 0;
                    /* find first '[' */
                    while (*p && *p != '[') p++;
                    if (*p == '[') {
                        p++;
                        rows = (int)strtol(p, (char**)&p, 10);
                        /* find next '[' */
                        while (*p && *p != '[') p++;
                        if (*p == '[') {
                            p++;
                            cols = (int)strtol(p, (char**)&p, 10);
                        }
                    }
                    if (rows > 0 && cols > 0) {
                        required_words += rows * cols;
                    } else {
                        /* If parsing fails, conservatively treat as zero and report error later in process_data_directives */
                    }
                }
            }

            if (required_words > 0) {
                data_image = (data_word*)malloc(sizeof(data_word) * required_words);
                if (!data_image) {
                    error_report_ex(ERR_SEV_ERROR, ERR_OUT_OF_MEMORY, file_name, 0, "data image");
                    free(dataDirectives);
                    free(code);
                    free(entries);
                    free(externs);
                    return 1;
                }
                /* initialize to zero for safety */
                { int z; for (z = 0; z < required_words; z++) { data_image[z].value = 0; data_image[z].src_line = 0; } }
            }
        }

        process_data_directives(dataDirectives, dataDirectivesCount, data_base_addr, &data_image, &data_count, &DC, &label_table_head);
    }
    if (!check_duplicate_labels(label_table_head)) {
        error_found = 1;
    }
    if (error_found || error_get_error_count() > 0) {
        error_report_ex(ERR_SEV_ERROR, ERR_PASS1_FAILED, file_name, 0, NULL);
        free_label_list(label_table_head);
        free(code);
        free(data_image);
        free(entries);
        free(externs);
        free(dataDirectives);
        return 1;
    }
    {
        char basefile[128];
        get_basefile(file_name, basefile, sizeof(basefile));
        exe_second_pass(code, code_count, label_table_head, entries, entries_count, externs, externs_count, basefile);
        write_code_file(basefile, code, code_count, data_image, data_count);
        print_memory_map(code_count, code, data_count, data_image, label_table_head);
        print_symbol_table(label_table_head);
    }
    free_label_list(label_table_head);
    free(code);
    free(data_image);
    free(entries);
    free(externs);
    free(dataDirectives);
    return 0;
}

