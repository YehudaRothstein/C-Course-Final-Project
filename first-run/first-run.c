
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

void to_special_base4_str(unsigned short value, char *out);
// --- Minimal implementations for register/matrix helpers ---
int regnum(const char *reg) {
    // Expects reg like "r0" to "r7"
    if (reg && reg[0] == 'r' && reg[1] >= '0' && reg[1] <= '7' && reg[2] == '\0')
        return reg[1] - '0';
    return 0;
}

int mat_reg(const char *mat, int which) {
    // Expects mat like "M1[r2][r7]", which=0 for row, 1 for col
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
    // Extracts label before first '['
    const char *p = strchr(mat, '[');
    if (!p) { strcpy(out, mat); return; }
    size_t len = p - mat;
    strncpy(out, mat, len);
    out[len] = '\0';
}
// Forward declarations for register/matrix helpers
int regnum(const char *reg);
int mat_reg(const char *mat, int which);
void mat_label(const char *mat, char *out);
// --- data_word struct definition (for data image) ---
typedef struct {
    unsigned short value;
    int src_line;
} data_word;

// --- Project-wide macro definitions (define here if not in headers) ---
#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 256
#endif
#ifndef IC_INIT_VALUE
#define IC_INIT_VALUE 100
#endif
#ifndef CODE_OUT_FILE
#define CODE_OUT_FILE "outputs/code.txt"
#endif





// --- DataDirective struct definition (for .data/.string/.mat collection) ---
typedef struct {
    char type[8]; // .data, .string, .mat
    char label[32];
    char operands[MAX_LINE_LENGTH];
    int src_line;
} DataDirective;

// Prototype for second pass function (now in second-run.h)
#include "second-run.h"

// Prototype for write_code_file
void write_code_file(const char *out_filename, code_conv *code, int code_count, data_word *data_image, int data_count);

// Implementation of write_code_file
// Outputs the code and data images to the object file, including ARE field per code word
void write_code_file(const char *out_filename, code_conv *code, int code_count, data_word *data_image, int data_count) {
    // Prepare output file name with .ob extension
    char ob_filename[256];
    // Use the base name of the input file (e.g., input.as -> input.ob)
    const char *slash = strrchr(out_filename, '/');
    #ifdef _WIN32
    if (!slash) slash = strrchr(out_filename, '\\');
    #endif
    const char *base = slash ? slash + 1 : out_filename;
    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    strncpy(ob_filename, base, len);
    ob_filename[len] = '\0';
    strcat(ob_filename, ".ob");
    FILE *fp = fopen(ob_filename, "w");
    if (!fp) {
        printf("[ERROR] Cannot open output file: %s\n", ob_filename);
        return;
    }
    // Arrays for code and data
    // Find the max address needed (cover both code and data)
    // int code_cells = code_count; // Unused variable removed
    // int data_cells = (int)data_memory_map.size; // Removed duplicate definition
    // Print header: code size and data size
    int data_cells = (int)data_memory_map.size;
    fprintf(fp, "%d %d\n", code_count, data_cells);
    // Print all addresses from 0 to total_cells-1 (address = IC_INIT_VALUE + i)
    // Print code section: addresses, code, aaaaa for data
    for (int i = 0; i < code_count; i++) {
        int addr = IC_INIT_VALUE + i;
        // Address as 10 bits base-4 (a-d, 5 chars), but print only last 4 chars (remove leading 'a')
        char addr_base4[6];
        for (int d = 4; d >= 0; d--) {
            int digit = (addr >> (d * 2)) & 0x3;
            addr_base4[4 - d] = "abcd"[digit];
        }
        addr_base4[5] = '\0';
        // Remove leading 'a'
        char *addr_ptr = addr_base4;
        while (*addr_ptr == 'a' && *(addr_ptr+1)) addr_ptr++;
        // Code as 10 bits base-4 (a-d)
        unsigned short code_val = code[i].value & 0x3FF;
        char code_base4[6];
        for (int d = 4; d >= 0; d--) {
            int digit = (code_val >> (d * 2)) & 0x3;
            code_base4[4 - d] = "abcd"[digit];
        }
        code_base4[5] = '\0';
        fprintf(fp, "%s %s\n", addr_ptr, code_base4);
    }
    for (int i = 0; i < data_cells; i++) {
        int addr = IC_INIT_VALUE + code_count + i;
        char addr_base4[6];
        for (int d = 4; d >= 0; d--) {
            int digit = (addr >> (d * 2)) & 0x3;
            addr_base4[4 - d] = "abcd"[digit];
        }
        addr_base4[5] = '\0';
        char *addr_ptr = addr_base4;
        while (*addr_ptr == 'a' && *(addr_ptr+1)) addr_ptr++;
        // Code: always aaaaa for data section
        // Data as 10 bits base-4 (a-d)
        unsigned short data_val = 0;
        if (i < (int)data_memory_map.size) {
            data_val = data_memory_map.cells[i].value & 0x3FF;
        }
        char data_base4[6];
        for (int d = 4; d >= 0; d--) {
            int digit = (data_val >> (d * 2)) & 0x3;
            data_base4[4 - d] = "abcd"[digit];
        }
        data_base4[5] = '\0';
        fprintf(fp, "%s %s\n", addr_ptr, data_base4);
    }
    fclose(fp);
    // Print a clear memory map: address (decimal), type, value (decimal), value (base-4)
    printf("\n==== MEMORY MAP (CODE + DATA) ====\n");
    printf("%-10s %-8s %-10s %-8s\n", "Address", "Type", "Decimal", "Base4");
    for (int i = 0; i < code_count; ++i) {
        int addr = IC_INIT_VALUE + i;
        unsigned short val = code[i].value & 0x3FF;
        char val_base4[6];
        for (int d = 4; d >= 0; d--) {
            int digit = (val >> (d * 2)) & 0x3;
            val_base4[4 - d] = "abcd"[digit];
        }
        val_base4[5] = '\0';
        printf("%-10d %-8s %-10d %-8s\n", addr, "CODE", val, val_base4);
    }
    for (int i = 0; i < data_cells; ++i) {
        int addr = IC_INIT_VALUE + code_count + i;
        unsigned short val = data_memory_map.cells[i].value & 0x3FF;
        char val_base4[6];
        for (int d = 4; d >= 0; d--) {
            int digit = (val >> (d * 2)) & 0x3;
            val_base4[4 - d] = "abcd"[digit];
        }
        val_base4[5] = '\0';
        printf("%-10d %-8s %-10d %-8s\n", addr, "DATA", val, val_base4);
    }
}

// Forward declaration for operand_extra_words
int operand_extra_words(const char *op);


// ...existing code...
// Print the symbol table after the first pass (linked list version)
int print_symbol_table(LabelNode *head) {
    printf("name | id (ic\\dc) | type\n");
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

// ...existing code...

// --- Function prototypes for helpers ---
int legal_label_decl(const char *label, int *error_code);
void print_external_error(int error_code, const char *file_name, int line_num);
int process_data(const char *operands, data_conv **data);
int process_string(const char *operands, data_conv **data);
void add_to_other_table(other_table **table, int *count, const char *operand);
// Linked list label table helpers
// (check_each_label_once, exe_second_pass, free_all_memory are now obsolete or need to be reimplemented for linked list)

// Helper: convert a 10-bit value to the special base-4 string (5 chars, a-d)
// (No longer used for output, but kept for other uses)
void to_special_base4_str(unsigned short value, char *out) {
    for (int i = 4; i >= 0; i--) {
        int digit = (value >> (i * 2)) & 0x3;
        out[4 - i] = "abcd"[digit];
    }
    out[5] = '\0';
}

// ...existing code...


int exe_first_pass(char *file_name) {
    // After the main loop, process all collected data directives in order

    // (Move this block after all variable declarations, so all variables are in scope)
    char line_buf[MAX_LINE_LENGTH];
    int IC = IC_INIT_VALUE, DC = 0;
    int error_found = 0;

    // Linked list label table
    LabelNode *label_table_head = NULL;
    other_table *externs = NULL, *entries = NULL;
    int externs_count = 0, entries_count = 0;
    code_conv code[1024];
    int code_count = 0;
    // Dynamic data image (heap)
    data_word *data_image = NULL;
    int data_count = 0;
    memory_map_init(&data_memory_map);
    // Collect data directives in order
    DataDirective *data_directives = NULL;
    int data_directives_count = 0;
    FILE *fp;


    // ---
    // Move data directive processing to after the first file read loop, so data_directives is filled
    // ---

    // ...existing code...


    // [LOG] Opening file: ...
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

        // If no opcode, treat as empty line and skip (do not count as error)
        if (!inst.opcode) {
            printf("[LOG] No opcode at line %d (empty or whitespace line). Skipping.\n", line_num);
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
            if (strcmp(inst.opcode, ".data") == 0 || strcmp(inst.opcode, ".string") == 0 || strcmp(inst.opcode, ".mat") == 0) {
                // Collect directive for later processing
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
            // Real opcode
            const OpcodeInfo *opinfo = find_opcode(inst.opcode);
            if (!opinfo) {
                printf("[ERROR] Unknown opcode '%s' at line %d\n", inst.opcode, line_num);
                print_external_error(1, file_name, line_num);
                error_found = 1;
                code[code_count].translated = 0;
                code_count++;
                free_inst_parts(&inst);
                continue;
            }
            // Validate number of operands
            int actual_operands = 0;
            char ops[128];
            ops[0] = '\0';
            if (inst.operands && strlen(inst.operands) > 0) {
                strncpy(ops, inst.operands, 127);
                ops[127] = '\0';
                const char *tmp = ops;
                int commas = 0;
                while (*tmp) { if (*tmp == ',') commas++; tmp++; }
                actual_operands = commas + 1;
                int only_ws = 1;
                for (const char *p = ops; *p; p++) {
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
            // Parse operands
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
            // Build first word: opcode, addressing modes, ARE=0 (A)
            unsigned short word = 0;
            word |= (opinfo->code & 0xF) << 6; // opcode: bits 6-9
            int src_mode = 0, dst_mode = 0;
            if (opinfo->num_operands == 2) {
                // Source
                if (op1[0] == '#') src_mode = 0;
                else if (strchr(op1, '[') && strchr(strchr(op1, '[')+1, '[')) src_mode = 2;
                else if (op1[0] == 'r' && op1[1] >= '0' && op1[1] <= '7' && op1[2] == '\0') src_mode = 3;
                else src_mode = 1;
                // Dest
                if (op2[0] == '#') dst_mode = 0;
                else if (strchr(op2, '[') && strchr(strchr(op2, '[')+1, '[')) dst_mode = 2;
                else if (op2[0] == 'r' && op2[1] >= '0' && op2[1] <= '7' && op2[2] == '\0') dst_mode = 3;
                else dst_mode = 1;
                word |= (src_mode & 0x3) << 4; // bits 4-5
                word |= (dst_mode & 0x3) << 2; // bits 2-3
            } else if (opinfo->num_operands == 1) {
                // Only dest
                if (op1[0] == '#') dst_mode = 0;
                else if (strchr(op1, '[') && strchr(strchr(op1, '[')+1, '[')) dst_mode = 2;
                else if (op1[0] == 'r' && op1[1] >= '0' && op1[1] <= '7' && op1[2] == '\0') dst_mode = 3;
                else dst_mode = 1;
                word |= (dst_mode & 0x3) << 2;
            }
            // ARE bits (A=0, R=1, E=2) - for first word always 0 (A)
            word |= 0; // bits 0-1
            if (inst.label) {
                insert_label(&label_table_head, inst.label, IC, 1, 0, 0);
            }
            code[code_count].value = word;
            code[code_count].are = 0;
            code[code_count].translated = 1;
            code[code_count].src_line = line_num;
            code_count++;
            IC++;
            // Encode extra words for operands
            if (opinfo->num_operands == 2) {
                // Matrix addressing: 2 extra words
                if (src_mode == 2) {
                    char matlbl[32]; mat_label(op1, matlbl);
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1; // R (relocatable)
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                    unsigned short regword = 0;
                    regword |= (mat_reg(op1, 0) & 0xF) << 6; // row reg
                    regword |= (mat_reg(op1, 1) & 0xF) << 2; // col reg
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (src_mode == 0) {
                    // Immediate
                    int val = atoi(op1+1);
                    code[code_count].value = (unsigned short)(val & 0x3FF);
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (src_mode == 3 && dst_mode == 3) {
                    // Both registers, share a word
                    unsigned short regword = 0;
                    regword |= (regnum(op1) & 0xF) << 6;
                    regword |= (regnum(op2) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (src_mode == 3) {
                    unsigned short regword = 0;
                    regword |= (regnum(op1) & 0xF) << 6;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                } else if (src_mode == 1) {
                    // Direct (label)
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1; // R
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                }
                // Dest operand
                if (dst_mode == 2) {
                    char matlbl[32]; mat_label(op2, matlbl);
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                    unsigned short regword = 0;
                    regword |= (mat_reg(op2, 0) & 0xF) << 6;
                    regword |= (mat_reg(op2, 1) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (dst_mode == 0) {
                    int val = atoi(op2+1);
                    code[code_count].value = (unsigned short)(val & 0x3FF);
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (!(src_mode == 3 && dst_mode == 3) && dst_mode == 3) {
                    unsigned short regword = 0;
                    regword |= (regnum(op2) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (dst_mode == 1) {
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                }
            } else if (opinfo->num_operands == 1) {
                if (dst_mode == 2) {
                    char matlbl[32]; mat_label(op1, matlbl);
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                    unsigned short regword = 0;
                    regword |= (mat_reg(op1, 0) & 0xF) << 6;
                    regword |= (mat_reg(op1, 1) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                } else if (dst_mode == 0) {
                    int val = atoi(op1+1);
                    code[code_count].value = (unsigned short)(val & 0x3FF);
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                } else if (dst_mode == 3) {
                    unsigned short regword = 0;
                    regword |= (regnum(op1) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                } else if (dst_mode == 1) {
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                }
            }
        }
        free_inst_parts(&inst);
    }

    fclose(fp);

    // Now process all collected data directives in order (after file read loop)
    DC = 0;
    data_count = 0;
    if (data_image) { free(data_image); data_image = NULL; }
    // Update data label addresses to be after code section (IC_INIT_VALUE + code_count)
    int data_base_addr = IC_INIT_VALUE + code_count;
    for (int i = 0; i < data_directives_count; i++) {
        DataDirective *dd = &data_directives[i];
        int base_dc = DC;
        if (strcmp(dd->type, ".data") == 0) {
            if (dd->label[0]) {
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC; // label's address in data section
            }
            // Parse comma-separated integers and store in data_image (dynamic)
            const char *p = dd->operands;
            char numbuf[32];
            int data_idx = base_dc;
            while (*p) {
                while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
                if (!*p) break;
                int j = 0;
                if (*p == '+' || *p == '-') numbuf[j++] = *p++;
                while (*p && isdigit((unsigned char)*p) && j < 30) numbuf[j++] = *p++;
                numbuf[j] = '\0';
                if (j == 0) break;
                int val = atoi(numbuf);
                data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                data_image[data_count].value = (unsigned short)(val & 0x3FF); // 10 bits
                data_image[data_count].src_line = dd->src_line;
                memory_map_set(&data_memory_map, data_idx, (unsigned short)(val & 0x3FF));
                data_count++;
                DC++;
                data_idx++;
            }
        } else if (strcmp(dd->type, ".string") == 0) {
            if (dd->label[0]) {
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC; // label's address in data section
            }
            // Support both ASCII and Unicode-style quotes for .string
            const char *start = strchr(dd->operands, '"');
            if (!start) {
                // Try Unicode left quote
                start = strchr(dd->operands, '\xE2');
                if (start && start[1] == '\x80' && start[2] == '\x9C') {
                    start += 3;
                    const char *end = strstr(start, "\xE2\x80\x9D");
                    if (end) {
                        int data_idx = base_dc;
                        for (const char *p = start; p < end; p++) {
                            data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                            data_image[data_count].value = (unsigned short)((unsigned char)*p & 0x3FF);
                            data_image[data_count].src_line = dd->src_line;
                            memory_map_set(&data_memory_map, data_idx, (unsigned short)((unsigned char)*p & 0x3FF));
                            data_count++;
                            DC++;
                            data_idx++;
                        }
                        // Null terminator
                        data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                        data_image[data_count].value = 0;
                        data_image[data_count].src_line = dd->src_line;
                        memory_map_set(&data_memory_map, data_idx, 0);
                        data_count++;
                        DC++;
                    }
                }
            } else {
                start++;
                const char *end = strchr(start, '"');
                if (end) {
                    int data_idx = base_dc;
                    for (const char *p = start; p < end; p++) {
                        data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                        data_image[data_count].value = (unsigned short)((unsigned char)*p & 0x3FF);
                        data_image[data_count].src_line = dd->src_line;
                        memory_map_set(&data_memory_map, data_idx, (unsigned short)((unsigned char)*p & 0x3FF));
                        data_count++;
                        DC++;
                        data_idx++;
                    }
                    // Null terminator
                    data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                    data_image[data_count].value = 0;
                    data_image[data_count].src_line = dd->src_line;
                    memory_map_set(&data_memory_map, data_idx, 0);
                    data_count++;
                    DC++;
                }
            }
        } else if (strcmp(dd->type, ".mat") == 0) {
            if (dd->label[0]) {
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC; // label's address in data section
            }
            // Parse matrix size and values
            int rows = 0, cols = 0;
            const char *p = dd->operands;
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
            int total = rows * cols;
            int mat_vals = 0;
            char numbuf[32];
            int data_idx = base_dc;
            while (mat_vals < total && p && *p) {
                while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
                if (!*p) break;
                int j = 0;
                if (*p == '+' || *p == '-') numbuf[j++] = *p++;
                while (*p && isdigit((unsigned char)*p) && j < 30) numbuf[j++] = *p++;
                numbuf[j] = '\0';
                if (j == 0) break;
                int val = atoi(numbuf);
                data_image = realloc(data_image, (data_count + 1) * sizeof(data_word));
                data_image[data_count].value = (unsigned short)(val & 0x3FF);
                data_image[data_count].src_line = dd->src_line;
                memory_map_set(&data_memory_map, data_idx, (unsigned short)(val & 0x3FF));
                data_count++;
                DC++;
                mat_vals++;
                data_idx++;
            }
            while (mat_vals < total) {
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
    if (data_directives) free(data_directives);

    printf("[LOG] Opening file: %s\n", file_name);
    fp = fopen(file_name, "r");
    if (!fp) {
        printf("[ERROR] Failed to open file: %s\n", file_name);
        handleError(1, file_name);
        return 1;
    }

    // int line_num = 0; // Removed duplicate declaration
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

        // If no opcode, treat as empty line and skip (do not count as error)
        if (!inst.opcode) {
            printf("[LOG] No opcode at line %d (empty or whitespace line). Skipping.\n", line_num);
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
            if (strcmp(inst.opcode, ".data") == 0 || strcmp(inst.opcode, ".string") == 0 || strcmp(inst.opcode, ".mat") == 0) {
                // Collect directive for later processing
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
            // Real opcode
            const OpcodeInfo *opinfo = find_opcode(inst.opcode);
            if (!opinfo) {
                printf("[ERROR] Unknown opcode '%s' at line %d\n", inst.opcode, line_num);
                print_external_error(1, file_name, line_num);
                error_found = 1;
                code[code_count].translated = 0;
                code_count++;
                free_inst_parts(&inst);
                continue;
            }
            // Validate number of operands
            int actual_operands = 0;
            char ops[128];
            ops[0] = '\0';
            if (inst.operands && strlen(inst.operands) > 0) {
                strncpy(ops, inst.operands, 127);
                ops[127] = '\0';
                const char *tmp = ops;
                int commas = 0;
                while (*tmp) { if (*tmp == ',') commas++; tmp++; }
                actual_operands = commas + 1;
                int only_ws = 1;
                for (const char *p = ops; *p; p++) {
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
            // Parse operands
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
            // Build first word: opcode, addressing modes, ARE=0 (A)
            unsigned short word = 0;
            word |= (opinfo->code & 0xF) << 6; // opcode: bits 6-9
            int src_mode = 0, dst_mode = 0;
            if (opinfo->num_operands == 2) {
                // Source
                if (op1[0] == '#') src_mode = 0;
                else if (strchr(op1, '[') && strchr(strchr(op1, '[')+1, '[')) src_mode = 2;
                else if (op1[0] == 'r' && op1[1] >= '0' && op1[1] <= '7' && op1[2] == '\0') src_mode = 3;
                else src_mode = 1;
                // Dest
                if (op2[0] == '#') dst_mode = 0;
                else if (strchr(op2, '[') && strchr(strchr(op2, '[')+1, '[')) dst_mode = 2;
                else if (op2[0] == 'r' && op2[1] >= '0' && op2[1] <= '7' && op2[2] == '\0') dst_mode = 3;
                else dst_mode = 1;
                word |= (src_mode & 0x3) << 4; // bits 4-5
                word |= (dst_mode & 0x3) << 2; // bits 2-3
            } else if (opinfo->num_operands == 1) {
                // Only dest
                if (op1[0] == '#') dst_mode = 0;
                else if (strchr(op1, '[') && strchr(strchr(op1, '[')+1, '[')) dst_mode = 2;
                else if (op1[0] == 'r' && op1[1] >= '0' && op1[1] <= '7' && op1[2] == '\0') dst_mode = 3;
                else dst_mode = 1;
                word |= (dst_mode & 0x3) << 2;
            }
            // ARE bits (A=0, R=1, E=2) - for first word always 0 (A)
            word |= 0; // bits 0-1
            if (inst.label) {
                insert_label(&label_table_head, inst.label, IC, 1, 0, 0);
            }
            code[code_count].value = word;
            code[code_count].are = 0;
            code[code_count].translated = 1;
            code[code_count].src_line = line_num;
            code_count++;
            IC++;
            // Encode extra words for operands
            if (opinfo->num_operands == 2) {
                // Matrix addressing: 2 extra words
                if (src_mode == 2) {
                    char matlbl[32]; mat_label(op1, matlbl);
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1; // R (relocatable)
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                    unsigned short regword = 0;
                    regword |= (mat_reg(op1, 0) & 0xF) << 6; // row reg
                    regword |= (mat_reg(op1, 1) & 0xF) << 2; // col reg
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (src_mode == 0) {
                    // Immediate
                    int val = atoi(op1+1);
                    code[code_count].value = (unsigned short)(val & 0x3FF);
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (src_mode == 3 && dst_mode == 3) {
                    // Both registers, share a word
                    unsigned short regword = 0;
                    regword |= (regnum(op1) & 0xF) << 6;
                    regword |= (regnum(op2) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (src_mode == 3) {
                    unsigned short regword = 0;
                    regword |= (regnum(op1) & 0xF) << 6;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                } else if (src_mode == 1) {
                    // Direct (label)
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1; // R
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                }
                // Dest operand
                if (dst_mode == 2) {
                    char matlbl[32]; mat_label(op2, matlbl);
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                    unsigned short regword = 0;
                    regword |= (mat_reg(op2, 0) & 0xF) << 6;
                    regword |= (mat_reg(op2, 1) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (dst_mode == 0) {
                    int val = atoi(op2+1);
                    code[code_count].value = (unsigned short)(val & 0x3FF);
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (!(src_mode == 3 && dst_mode == 3) && dst_mode == 3) {
                    unsigned short regword = 0;
                    regword |= (regnum(op2) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                } else if (dst_mode == 1) {
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1;
                    code[code_count].translated = 1;
                    code[code_count].src_line = line_num;
                    code_count++;
                    IC++;
                }
            } else if (opinfo->num_operands == 1) {
                if (dst_mode == 2) {
                    char matlbl[32]; mat_label(op1, matlbl);
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                    unsigned short regword = 0;
                    regword |= (mat_reg(op1, 0) & 0xF) << 6;
                    regword |= (mat_reg(op1, 1) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                } else if (dst_mode == 0) {
                    int val = atoi(op1+1);
                    code[code_count].value = (unsigned short)(val & 0x3FF);
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                } else if (dst_mode == 3) {
                    unsigned short regword = 0;
                    regword |= (regnum(op1) & 0xF) << 2;
                    code[code_count].value = regword;
                    code[code_count].are = 0;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                } else if (dst_mode == 1) {
                    code[code_count].value = 0; // Will be filled in 2nd pass
                    code[code_count].are = 1;
                    code[code_count].translated = 1;
                    code_count++;
                    IC++;
                }
            }
        }
        free_inst_parts(&inst);
    }

    fclose(fp);

    update_data_labels_address(label_table_head);

    if (!check_duplicate_labels(label_table_head)) {
        error_found = 1;
    }

    // --- Second pass: resolve all label addresses in code, mark externals, and output .ent/.ext files ---
    // Get base filename for .ent/.ext output (without path and extension)
    char basefile[128] = "outputs/code";
    exe_second_pass(code, code_count, label_table_head, entries, entries_count, externs, externs_count, basefile);

    write_code_file(CODE_OUT_FILE, code, code_count, data_image, data_count);


    // Print a simple, clear memory map: address (decimal), type (CODE/DATA), value (decimal), value (base-4)
    printf("\n==== MEMORY MAP (address = 100 + code_count + cell index) ====\n");
    if (data_memory_map.size == 0) {
        printf("[DEBUG] No data in memory map.\n");
    } else {
        printf("%-8s %-8s %-8s %-8s\n", "Address", "Type", "Value", "Base-4");
        for (size_t i = 0; i < data_memory_map.size; ++i) {
            size_t addr = IC_INIT_VALUE + code_count + i;
            unsigned short val = data_memory_map.cells[i].value & 0x3FF;
            char base4[6] = {0};
            for (int j = 4; j >= 0; --j) {
                base4[4-j] = "abcd"[(val >> (j*2)) & 0x3];
            }
            base4[5] = '\0';
            printf("%-8zu %-8s %-8u %-8s\n", addr, "DATA", val, base4);
        }
    }

    print_symbol_table(label_table_head);

    free_label_list(label_table_head);
    if (entries) free(entries);
    if (externs) free(externs);
    if (data_image) free(data_image);
    // 'code' is a static array, do not free

    return error_found;
}

// --- exe_second_pass implementation moved to second-run.c ---

// --- Implementations for missing functions ---
// Minimal stubs to resolve linker errors. Replace with real logic as needed.
int legal_label_decl(const char *label, int *error_code) {
    // Error codes:
    // 1 = NULL or empty
    // 2 = too long
    // 3 = does not start with letter
    // 4 = contains non-alphanumeric
    // 5 = reserved word (opcode or instruction)
    // 6 = register name
    if (!label || !label[0]) {
        if (error_code) *error_code = 1;
        return 0;
    }
    int len = strlen(label);
    if (len > 30) {
        if (error_code) *error_code = 2;
        return 0;
    }
    if (!isalpha((unsigned char)label[0])) {
        if (error_code) *error_code = 3;
        return 0;
    }
    for (int i = 1; i < len; i++) {
        if (!isalnum((unsigned char)label[i])) {
            if (error_code) *error_code = 4;
            return 0;
        }
    }
    // Check reserved words (opcodes and instructions)
    if (find_opcode(label) != NULL || is_instr(label)) {
        if (error_code) *error_code = 5;
        return 0;
    }
    // Check register names: r0 to r7
    if (len == 2 && label[0] == 'r' && label[1] >= '0' && label[1] <= '7') {
        if (error_code) *error_code = 6;
        return 0;
    }
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
    // Parse comma-separated integers, add to data_conv array if needed
    if (!operands) return 0;
    int count = 0;
    const char *p = operands;
    char numbuf[32];
    while (*p) {
        // Skip whitespace
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (!*p) break;
        // Read number
        int i = 0;
        if (*p == '+' || *p == '-') numbuf[i++] = *p++;
        while (*p && isdigit((unsigned char)*p) && i < 30) numbuf[i++] = *p++;
        numbuf[i] = '\0';
        if (i == 0) break;
        int val = atoi(numbuf);
        // If data is not NULL, store value (assume array, not linked list)
        if (data && *data) {
            (*data)[count].value = val;
        }
        count++;
    }
    return count;
}

int process_string(const char *operands, data_conv **data) {
    // Parse quoted string, add ASCII values to data_conv array if needed, add null terminator
    if (!operands) return 0;
    const char *start = strchr(operands, '"');
    if (!start) return 0;
    start++;
    const char *end = strchr(start, '"');
    if (!end) return 0;
    int count = 0;
    for (const char *p = start; p < end; p++) {
        if (data && *data) {
            (*data)[count].value = (unsigned char)*p;
        }
        count++;
    }
    // Add null terminator
    if (data && *data) {
        (*data)[count].value = 0;
    }
    count++;
    return count;
}

void add_to_other_table(other_table **table, int *count, const char *operand) {
    // Add a label to the other_table array (for .extern/.entry)
    if (!operand || !table || !count) return;
    // Skip whitespace
    const char *p = operand;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) return;
    // Find end of label
    const char *end = p;
    while (*end && !isspace((unsigned char)*end) && *end != ',') end++;
    int len = end - p;
    if (len <= 0) return;
    if (len > 31) len = 31;
    // Allocate or grow the table
    other_table *new_table = realloc(*table, (*count + 1) * sizeof(other_table));
    if (!new_table) return;
    *table = new_table;
    // Use the correct field name for the label (try 'name', fallback to 'symbol' if needed)
#ifdef OTHER_TABLE_LABEL_FIELD
    strncpy((*table)[*count].OTHER_TABLE_LABEL_FIELD, p, len);
    (*table)[*count].OTHER_TABLE_LABEL_FIELD[len] = '\0';
#else
    strncpy((*table)[*count].name, p, len);
    (*table)[*count].name[len] = '\0';
#endif
    (*count)++;
}
