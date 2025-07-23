
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// Helper: count extra words for operands based on addressing mode
// Returns the number of extra words for a single operand
int operand_extra_words(const char *operand) {
    if (!operand) return 0;
    // Immediate addressing: #number
    if (operand[0] == '#') return 1;
    // Matrix addressing: label[rX][rY]
    const char *lbracket = strchr(operand, '[');
    if (lbracket && strchr(lbracket+1, '[')) return 2;
    // Register direct: r0-r7 (but if both operands are registers, only 1 extra word for both)
    if (operand[0] == 'r' && operand[1] >= '0' && operand[1] <= '7' && operand[2] == '\0') return 1;
    // Direct addressing (label)
    return 1;
}

// Helper: count total words for an instruction (opcode + operands)
#include "code_conversion.h" // For OpcodeInfo
int instruction_word_count(const OpcodeInfo *opinfo, const char *operands) {
    if (!opinfo) return 1;
    int count = 1; // opcode word
    if (opinfo->num_operands == 0) return count;
    // Split operands
    char ops[128];
    if (operands) strncpy(ops, operands, 127); else ops[0] = '\0';
    ops[127] = '\0';
    char *op1 = NULL, *op2 = NULL;
    char *comma = strchr(ops, ',');
    if (comma) {
        *comma = '\0';
        op1 = ops;
        op2 = comma + 1;
        while (*op2 == ' ' || *op2 == '\t') op2++;
    } else {
        op1 = ops;
    }
    if (opinfo->num_operands == 2) {
        int w1 = operand_extra_words(op1);
        int w2 = operand_extra_words(op2);
        // If both operands are registers, only 1 extra word
        if (w1 == 1 && w2 == 1 && op1[0] == 'r' && op2[0] == 'r')
            count += 1;
        else
            count += w1 + w2;
    } else if (opinfo->num_operands == 1) {
        count += operand_extra_words(op1);
    }
    return count;
}
#include <ctype.h>
#include "label_table.h"
// Print the symbol table after the first pass
void print_symbol_table(Label *label_table, int label_table_line);
#include <stddef.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "utils.h"
#include "error-handler.h"
#include "label_table.h"
#include "other_table.h"
#include "data_conv.h"
#include "code_conversion.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef struct {
    unsigned short value;
    int are;         // 0=A, 1=R, 2=E
    int translated;  // 1 if translated, 0 if not
} code_conv;

// Print the symbol table after the first pass (implementation)
void print_symbol_table(Label *label_table, int label_table_line) {
    printf("name | id (ic\\dc) | type\n");
    for (int i = 0; i < label_table_line; i++) {
        const char *type = label_table[i].is_code ? "code" :
                           (label_table[i].is_data ? "data" :
                           (label_table[i].is_extern ? "extern" : "?"));
        printf("%s %d %s\n", label_table[i].name, label_table[i].address, type);
    }
}

// --- Function prototypes for helpers ---
int legal_label_decl(const char *label, int *error_code);
void print_external_error(int error_code, const char *file_name, int line_num);
int process_data(const char *operands, data_conv **data);
int process_string(const char *operands, data_conv **data);
void add_to_other_table(other_table **table, int *count, const char *operand);
int check_each_label_once(Label *label_table, int label_table_line, const char *file_name);
void exe_second_pass(const char *file_name, Label *label_table, int IC, int DC, int label_table_line,
                     int externs_count, int entries_count, code_conv *code, data_conv *data,
                     other_table *externs, other_table *entries, int error_found);
void free_all_memory(code_conv *code, Label *label_table, other_table *entries, other_table *externs,
                     int code_size, int label_table_line, int entries_count, int externs_count);


#define IC_INIT_VALUE 100
#define MAX_LINE_LENGTH 81

#define CODE_OUT_FILE "outputs/code.txt"





void write_code_file(const char *out_filename, code_conv *code, int code_count) {
    FILE *out = fopen(out_filename, "w");
    if (!out) return;
    for (int i = 0; i < code_count; i++) {
        char bin[15];
        if (code[i].translated) {
            to_binary_str(code[i].value, bin, 14);
            fprintf(out, "%s %c\n", bin, get_are_char(code[i].are));
        } else {
            fprintf(out, "? ?\n");
        }
    }
    fclose(out);
}

int exe_first_pass(char *file_name) {
    char line_buf[MAX_LINE_LENGTH];
    int IC = IC_INIT_VALUE, DC = 0;
    int error_found = 0;

    // Dynamic tables
    Label *label_table = NULL;
    int label_table_line = 0;
    other_table *externs = NULL, *entries = NULL;
    int externs_count = 0, entries_count = 0;
    code_conv code[1024];
    int code_count = 0;
    data_conv *data = NULL;
    FILE *fp;

    printf("[LOG] Opening file: %s\n", file_name);
    fp = fopen(file_name, "r");
    if (!fp) {
        printf("[ERROR] Failed to open file: %s\n", file_name);
        handleError(1, file_name);
        return 1;
    }

    int line_num = 0;
    while (fgets(line_buf, MAX_LINE_LENGTH, fp)) {
        line_num++;
        printf("[LOG] Read line %d: %s", line_num, line_buf);

        // Skip empty/comment lines
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
        inst_parts inst = parse_inst_line(line_buf);
        printf("[LOG] Parsed line %d: label='%s', opcode='%s', operands='%s'\n", line_num,
            inst.label ? inst.label : "(null)",
            inst.opcode ? inst.opcode : "(null)",
            inst.operands ? inst.operands : "(null)");

        if (!inst.opcode) {
            printf("[ERROR] Failed to parse opcode at line %d. Skipping line.\n", line_num);
            error_found = 1;
            free_inst_parts(&inst);
            continue;
        }

        // Handle label
        if (inst.label) {
            printf("[LOG] Found label '%s' at line %d\n", inst.label, line_num);
            int error_code = 0;
            if (!legal_label_decl(inst.label, &error_code)) {
                printf("[ERROR] Illegal label '%s' at line %d\n", inst.label, line_num);
                print_external_error(error_code, file_name, line_num);
                error_found = 1;
                free_inst_parts(&inst);
                continue;
            }
        }

        // Instruction line (.data/.string/.extern/.entry)
        if (is_instr(inst.opcode)) {
            printf("[LOG] Found instruction '%s' at line %d\n", inst.opcode, line_num);
            if (strcmp(inst.opcode, ".data") == 0) {
                // Add label if exists
                if (inst.label) {
                    printf("[LOG] Allocating label_table for .data at line %d, size %d\n", line_num, label_table_line + 1);
                    Label *new_table = realloc(label_table, (label_table_line + 1) * sizeof(Label));
                    if (!new_table) {
                        printf("[ERROR] Failed to allocate label_table for .data at line %d\n", line_num);
                        exit(1);
                    }
                    label_table = new_table;
                    strncpy(label_table[label_table_line].name, inst.label, 31);
                    label_table[label_table_line].name[31] = '\0';
                    label_table[label_table_line].address = DC;
                    label_table[label_table_line].is_code = 0;
                    label_table[label_table_line].is_data = 1;
                    label_table[label_table_line].is_extern = 0;
                    label_table[label_table_line].is_entry = 0;
                    label_table_line++;
                }
                printf("[LOG] Processing .data operands at line %d\n", line_num);
                DC += process_data(inst.operands, &data);
            } else if (strcmp(inst.opcode, ".string") == 0) {
                if (inst.label) {
                    printf("[LOG] Allocating label_table for .string at line %d, size %d\n", line_num, label_table_line + 1);
                    Label *new_table = realloc(label_table, (label_table_line + 1) * sizeof(Label));
                    if (!new_table) {
                        printf("[ERROR] Failed to allocate label_table for .string at line %d\n", line_num);
                        exit(1);
                    }
                    label_table = new_table;
                    strncpy(label_table[label_table_line].name, inst.label, 31);
                    label_table[label_table_line].name[31] = '\0';
                    label_table[label_table_line].address = DC;
                    label_table[label_table_line].is_code = 0;
                    label_table[label_table_line].is_data = 1;
                    label_table[label_table_line].is_extern = 0;
                    label_table[label_table_line].is_entry = 0;
                    label_table_line++;
                }
                printf("[LOG] Processing .string operands at line %d\n", line_num);
                DC += process_string(inst.operands, &data);
            } else if (strcmp(inst.opcode, ".extern") == 0) {
                printf("[LOG] Processing .extern at line %d\n", line_num);
                add_to_other_table(&externs, &externs_count, inst.operands);
            } else if (strcmp(inst.opcode, ".entry") == 0) {
                printf("[LOG] Processing .entry at line %d\n", line_num);
                add_to_other_table(&entries, &entries_count, inst.operands);
            } else if (strcmp(inst.opcode, ".mat") == 0) {
                // TODO: handle .mat directive
            }
        } else {
            printf("[LOG] Found opcode '%s' at line %d\n", inst.opcode, line_num);
            const OpcodeInfo *opinfo = find_opcode(inst.opcode);
            if (!opinfo) {
                printf("[ERROR] Unknown opcode '%s' at line %d\n", inst.opcode, line_num);
                print_external_error(1, file_name, line_num);
                error_found = 1;
                // Add a code_conv entry with translated=0 for this line
                code[code_count].translated = 0;
                code_count++;
                continue;
            }
            // Validate number of operands
            int actual_operands = 0;
            if (inst.operands && strlen(inst.operands) > 0) {
                // Count commas
                const char *tmp = inst.operands;
                int commas = 0;
                while (*tmp) { if (*tmp == ',') commas++; tmp++; }
                actual_operands = commas + 1;
                // If only whitespace, treat as 0
                int only_ws = 1;
                for (const char *p = inst.operands; *p; p++) {
                    if (!isspace((unsigned char)*p) && *p != ',') { only_ws = 0; break; }
                }
                if (only_ws) actual_operands = 0;
            }
            if (actual_operands != opinfo->num_operands) {
                printf("[ERROR] Wrong number of operands for '%s' at line %d: expected %d, got %d\n", inst.opcode, line_num, opinfo->num_operands, actual_operands);
                error_found = 1;
                code[code_count].translated = 0;
                code_count++;
                free_inst_parts(&inst);
                continue;
            }
            // If valid:
            int words_for_inst = instruction_word_count(opinfo, inst.operands);
            if (inst.label) {
                printf("[LOG] Allocating label_table for opcode at line %d, size %d\n", line_num, label_table_line + 1);
                Label *new_table = realloc(label_table, (label_table_line + 1) * sizeof(Label));
                if (!new_table) {
                    printf("[ERROR] Failed to allocate label_table for opcode at line %d\n", line_num);
                    exit(1);
                }
                label_table = new_table;
                strncpy(label_table[label_table_line].name, inst.label, 31);
                label_table[label_table_line].name[31] = '\0';
                label_table[label_table_line].address = IC;
                label_table[label_table_line].is_code = 1;
                label_table[label_table_line].is_data = 0;
                label_table[label_table_line].is_extern = 0;
                label_table[label_table_line].is_entry = 0;
                label_table_line++;
            }
            printf("[LOG] Generating code word for opcode '%s' at line %d, words=%d\n", inst.opcode, line_num, words_for_inst);
            for (int w = 0; w < words_for_inst; w++) {
                unsigned short code_word = (w == 0) ? (opinfo->code << 10) : 0; // Only first word is opcode, rest are operand words (set to 0 for now)
                int are = 0; // Set ARE as needed (0=A)
                code[code_count].value = code_word;
                code[code_count].are = are;
                code[code_count].translated = 1;
                code_count++;
            }
            IC += words_for_inst;
        }
        free_inst_parts(&inst);
    }

    printf("[LOG] Finished reading file, closing file pointer\n");
    fclose(fp);

    printf("[LOG] Updating data label addresses\n");
    update_data_labels_address(label_table, label_table_line, IC);
// Update data label addresses by adding final IC, so output matches assembler spec


    printf("[LOG] Checking for duplicate labels\n");
    if (!check_each_label_once(label_table, label_table_line, file_name)) {
        error_found = 1;
    }

    printf("[LOG] Writing code output file\n");
    write_code_file(CODE_OUT_FILE, code, code_count);

    printf("[LOG] Calling second pass\n");
    exe_second_pass(file_name, label_table, IC, DC, label_table_line,
                    externs_count, entries_count, code, data, externs, entries, error_found);

    // Print symbol table after first pass
    print_symbol_table(label_table, label_table_line);

    printf("[LOG] Freeing all memory\n");
    free_all_memory(code, label_table, entries, externs,
                    IC - IC_INIT_VALUE, label_table_line, entries_count, externs_count);

    printf("[LOG] First pass complete, returning error_found=%d\n", error_found);
    return error_found;
}

// --- Implementations for missing functions ---

#include <ctype.h>

int handle_allocation(other_table **externs, other_table **entries, code_conv *code, data_conv **data) {
    *externs = NULL;
    *entries = NULL;
    *data = NULL;
    // code is a static array, no allocation needed
    return 0;
}

int is_reserved_word(const char *label) {
    // Check opcode table
    extern OpcodeInfo opcode_table[];
    extern int num_opcodes;
    for (int i = 0; i < num_opcodes; i++) {
        if (strcmp(label, opcode_table[i].name) == 0)
            return 1;
    }
    // Check register names
    for (int i = 0; i < 8; i++) {
        char reg[4];
        sprintf(reg, "r%d", i);
        if (strcmp(label, reg) == 0)
            return 1;
    }
    // Check directives
    const char *directives[] = {".data", ".string", ".mat", ".entry", ".extern"};
    for (int i = 0; i < 5; i++) {
        if (strcmp(label, directives[i]) == 0)
            return 1;
    }
    return 0;
}

int legal_label_decl(const char *label, int *error_code) {
    if (!label || !isalpha(label[0])) {
        *error_code = 2; // Must start with letter
        return 0;
    }
    if (strlen(label) > 30) {
        *error_code = 3; // Too long
        return 0;
    }
    if (is_reserved_word(label)) {
        *error_code = 4; // Reserved word
        return 0;
    }
    *error_code = 0;
    return 1;
}

void print_external_error(int error_code, const char *file_name, int line_num) {
    switch (error_code) {
        case 2:
            printf("Error in %s at line %d: Label must start with a letter.\n", file_name, line_num);
            break;
        case 3:
            printf("Error in %s at line %d: Label is too long (max 30 chars).\n", file_name, line_num);
            break;
        case 4:
            printf("Error in %s at line %d: Label is a reserved word.\n", file_name, line_num);
            break;
        default:
            printf("Error in %s at line %d: Invalid label.\n", file_name, line_num);
            break;
    }
}

int process_data(const char *operands, data_conv **data) {
    int count = 0;
    char *ops = strdup(operands);
    char *token = strtok(ops, ",");
    while (token) {
        while (isspace((unsigned char)*token)) token++;
        int value = atoi(token);
        data_conv *new_data = realloc(*data, (count + 1) * sizeof(data_conv));
        if (!new_data) {
            free(ops);
            return count;
        }
        *data = new_data;
        (*data)[count].value = value;
        (*data)[count].translated = 1;
        count++;
        token = strtok(NULL, ",");
    }
    free(ops);
    return count;
}

int process_string(const char *operands, data_conv **data) {
    int count = 0;
    const char *start = strchr(operands, '"');
    if (!start) return 0;
    start++;
    const char *end = strchr(start, '"');
    if (!end) return 0;
    for (const char *p = start; p < end; p++) {
        data_conv *new_data = realloc(*data, (count + 1) * sizeof(data_conv));
        if (!new_data) return count;
        *data = new_data;
        (*data)[count].value = (int)(unsigned char)(*p);
        (*data)[count].translated = 1;
        count++;
    }
    // Add null terminator
    data_conv *new_data = realloc(*data, (count + 1) * sizeof(data_conv));
    if (!new_data) return count;
    *data = new_data;
    (*data)[count].value = 0;
    (*data)[count].translated = 1;
    count++;
    return count;
}

void add_to_other_table(other_table **table, int *count, const char *operand) {
    if (!operand) return;
    other_table *new_table = realloc(*table, (*count + 1) * sizeof(other_table));
    if (!new_table) return;
    *table = new_table;
    strncpy((*table)[*count].name, operand, 31);
    (*table)[*count].name[31] = '\0';
    (*table)[*count].address = 0;
    (*count)++;
}

int check_each_label_once(Label *label_table, int label_table_line, const char *file_name) {
    for (int i = 0; i < label_table_line; i++) {
        for (int j = i + 1; j < label_table_line; j++) {
            if (strcmp(label_table[i].name, label_table[j].name) == 0) {
                printf("Error: Duplicate label '%s' in %s\n", label_table[i].name, file_name);
                return 0;
            }
        }
    }
    return 1;
}

void exe_second_pass(const char *file_name, Label *label_table, int IC, int DC, int label_table_line,
                     int externs_count, int entries_count, code_conv *code, data_conv *data,
                     other_table *externs, other_table *entries, int error_found) {
    // Empty stub as requested
}

void free_all_memory(code_conv *code, Label *label_table, other_table *entries, other_table *externs,
                     int code_size, int label_table_line, int entries_count, int externs_count) {
    // Free all dynamic memory
    if (label_table) free(label_table);
    if (entries) free(entries);
    if (externs) free(externs);
    // 'code' is a static array, do not free
}
