#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "process_data_directives.h"
#include "memory_map_data.h"
#include "memory_map.h"
#include "../../error-handler/error-handler.h"

/* מציאת מידות מטריצה מהטקסט */
static int parse_matrix_dimensions(const char *text, int *rows, int *cols) {
    const char *cursor = text;

    /* מוצא את ה '[' הראשון */
    while (*cursor && *cursor != '[') {
        cursor++;
    }
    /* אם לא נמצא '[', מחזירים 0 */
    if (*cursor != '[') {
        return 0;
    }
    cursor++;

    *rows = (int)strtol(cursor, (char**)&cursor, 10);

    /* locate second '[' */
    while (*cursor && *cursor != '[') {
        cursor++;
    }
    if (*cursor != '[') {
        return 0;
    }
    cursor++;

    *cols = (int)strtol(cursor, (char**)&cursor, 10);

    return (*rows > 0 && *cols > 0);
}

void process_data_directives(
    DataParts *data_directives,
    int data_directives_count,
    int data_base_addr,
    data_word **data_image_ptr,
    int *data_count_ptr,
    int *DC_ptr,
    LabelNode **label_table_head_ptr) {
    int data_counter = *DC_ptr;                 /* DC – מונה נתונים */
    int data_word_count = *data_count_ptr;      /* מספר המילים שהוקצו לנתונים */
    data_word *data_image_array = *data_image_ptr; /* מערך תמונת הנתונים */
    LabelNode *label_head = *label_table_head_ptr;  /* טבלת תוויות */
    int directive_index;

    for (directive_index = 0; directive_index < data_directives_count; directive_index++) {
        DataParts *directive = &data_directives[directive_index];
        int start_index = data_counter; /* תחילת הכתיבה של ההנחיה הנוכחית */

        if (strcmp(directive->type, ".data") == 0) {
            if (directive->label[0]) {
                insert_label(&label_head, directive->label, data_base_addr + data_counter, 0, 1, 0);
                start_index = data_counter;
            }
            {
                const char *ptr = directive->operands;
                int write_index = start_index;

                while (*ptr) {
                    while (*ptr && isspace((unsigned char)*ptr)) {
                        ptr++;
                    }
                    if (*ptr) {
                        char *endptr;
                        long value;

                        if (*ptr == ',') {
                            ptr++;
                        }
                        while (*ptr && isspace((unsigned char)*ptr)) {
                            ptr++;
                        }

                        value = strtol(ptr, &endptr, 10);
                        if (ptr != endptr) {
                            /* Address-based cap: last valid address is 255 (MEMORY_SIZE-1) */
                            if ((data_base_addr + data_counter) >= MEMORY_SIZE) {
                                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".data exceeds memory size");
                                return; /* stop further corruption */
                            }
                            data_image_array[write_index].value = (unsigned short)value;
                            data_image_array[write_index].src_line = directive->src_line;
                            write_index++;
                            data_counter++;
                            data_word_count++;
                        }
                        ptr = endptr;
                    }
                }
            }
        } else if (strcmp(directive->type, ".string") == 0) {
            if (directive->label[0]) {
                insert_label(&label_head, directive->label, data_base_addr + data_counter, 0, 1, 0);
                start_index = data_counter;
            }
            {
                const char *ptr2 = directive->operands;
                int write_index2 = start_index;

                while (*ptr2 && *ptr2 != '"') {
                    ptr2++;
                }
                if (*ptr2 == '"') {
                    ptr2++;
                    while (*ptr2 && *ptr2 != '"') {
                        unsigned char ch = (unsigned char)*ptr2; /* C89: declare at block start */
                        /* Enforce printable ASCII range 32..126 inclusive */
                        if (ch < 32 || ch > 126) {
                            error_report_ex(ERR_SEV_ERROR, ERR_STRING_BAD_CHAR, NULL, directive->src_line, NULL);
                            return;
                        }
                        if ((data_base_addr + data_counter) >= MEMORY_SIZE) {
                            error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".string exceeds memory size");
                            return;
                        }
                        data_image_array[write_index2].value = (unsigned short)ch;
                        data_image_array[write_index2].src_line = directive->src_line;
                        write_index2++;
                        data_counter++;
                        data_word_count++;
                        ptr2++;
                    }
                    /* null terminator */
                    if ((data_base_addr + data_counter) >= MEMORY_SIZE) {
                        error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".string exceeds memory size");
                        return;
                    }
                    data_image_array[write_index2].value = 0;
                    data_image_array[write_index2].src_line = directive->src_line;
                    write_index2++;
                    data_counter++;
                    data_word_count++;
                } else {
                    error_report_ex(ERR_SEV_ERROR, ERR_STRING_NOT_QUOTED, NULL, directive->src_line, NULL);
                    return;
                }
            }
        } else if (strcmp(directive->type, ".mat") == 0) {
            int rows = 0, cols = 0, total = 0;
            if (directive->label[0]) {
                /* mark as data, not extern */
                insert_label(&label_head, directive->label, data_base_addr + data_counter, 0, 1, 0);
                start_index = data_counter;
            }
            if (!parse_matrix_dimensions(directive->operands, &rows, &cols)) {
                error_report_ex(ERR_SEV_ERROR, ERR_MAT_SIZE_INVALID, NULL, directive->src_line, NULL);
                return;
            }
            total = rows * cols;
            {
                /* Move p to just after the SECOND closing ']' */
                const char *ptr3 = directive->operands;
                int close_brackets_seen = 0;
                while (*ptr3) {
                    if (*ptr3 == ']') {
                        close_brackets_seen++;
                        if (close_brackets_seen == 2) { ptr3++; break; }
                    }
                    ptr3++;
                }
                /* Skip whitespace and an optional comma */
                while (*ptr3 && isspace((unsigned char)*ptr3)) ptr3++;
                if (*ptr3 == ',') { ptr3++; }
                while (*ptr3 && isspace((unsigned char)*ptr3)) ptr3++;

                /* Parse up to total initializers; fill remaining with 0 */
                {
                    int filled = 0;
                    while (*ptr3 && filled < total) {
                        while (*ptr3 && isspace((unsigned char)*ptr3)) ptr3++;
                        if (!*ptr3) break;
                        if (*ptr3 == ',') { ptr3++; continue; }
                        {
                            char *endptr3;
                            long value3 = strtol(ptr3, &endptr3, 10);
                            if (ptr3 == endptr3) break; /* no more numbers */
                            if ((data_base_addr + data_counter) >= MEMORY_SIZE) {
                                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".mat exceeds memory size");
                                return;
                            }
                            data_image_array[start_index + filled].value = (unsigned short)value3;
                            data_image_array[start_index + filled].src_line = directive->src_line;
                            filled++;
                            data_counter++;
                            data_word_count++;
                            ptr3 = endptr3;
                            while (*ptr3 && isspace((unsigned char)*ptr3)) ptr3++;
                            if (*ptr3 == ',') ptr3++;
                        }
                    }
                    while (filled < total) {
                        if ((data_base_addr + data_counter) >= MEMORY_SIZE) {
                            error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".mat exceeds memory size");
                            return;
                        }
                        data_image_array[start_index + filled].value = 0;
                        data_image_array[start_index + filled].src_line = directive->src_line;
                        filled++;
                        data_counter++;
                        data_word_count++;
                    }
                    /* Warn on extra values */
                    while (*ptr3) {
                        while (*ptr3 && isspace((unsigned char)*ptr3)) ptr3++;
                        if (*ptr3 == ',') { ptr3++; continue; }
                        if (!*ptr3) break;
                        {
                            char *end2;
                            (void)strtol(ptr3, &end2, 10);
                            if (ptr3 != end2) {
                                error_report_ex(ERR_SEV_WARNING, ERR_MAT_INIT_COUNT_MISMATCH, NULL, directive->src_line, "extra initializers ignored");
                                break;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    *data_image_ptr = data_image_array;
    *data_count_ptr = data_word_count;
    *DC_ptr = data_counter;
    *label_table_head_ptr = label_head;
}
