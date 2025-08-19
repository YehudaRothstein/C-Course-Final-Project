#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "process_data_directives.h"
#include "memory_map_data.h"
#include "memory_map.h"
#include "../../error-handler/error-handler.h"


static int parse_dims(const char *s, int *rows, int *cols) {
    const char *p = s;
    /* find first '[' */
    while (*p && *p != '[') p++;
    if (*p != '[') return 0;
    p++;
    *rows = (int)strtol(p, (char**)&p, 10);
    while (*p && *p != '[') p++;
    if (*p != '[') return 0;
    p++;
    *cols = (int)strtol(p, (char**)&p, 10);
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
    int DC = *DC_ptr;
    int data_count = *data_count_ptr;
    data_word *data_image = *data_image_ptr;
    LabelNode *label_table_head = *label_table_head_ptr;
    int i;

    for (i = 0; i < data_directives_count; i++) {
        DataParts *dd = &data_directives[i];
        int base_dc = DC;

        if (strcmp(dd->type, ".data") == 0) {
            if (dd->label[0]) {
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC;
            }
            {
                const char *p = dd->operands;
                int data_idx = base_dc;
                while (*p) {
                    while (*p && isspace((unsigned char)*p)) p++;
                    if (*p) {
                        char *end;
                        long val;
                        if (*p == ',') p++;
                        while (*p && isspace((unsigned char)*p)) p++;
                        val = strtol(p, &end, 10);
                        if (p != end) {
                            /* Address-based cap: last valid address is 255 (MEMORY_SIZE-1) */
                            if ((data_base_addr + DC) >= MEMORY_SIZE) {
                                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, dd->src_line, ".data exceeds memory size");
                                return; /* stop further corruption */
                            }
                            data_image[data_idx].value = (unsigned short)val;
                            data_image[data_idx].src_line = dd->src_line;
                            data_idx++;
                            DC++;
                            data_count++;
                        }
                        p = end;
                    }
                }
            }
        } else if (strcmp(dd->type, ".string") == 0) {
            if (dd->label[0]) {
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC;
            }
            {
                const char *p = dd->operands;
                while (*p && *p != '"') p++;
                if (*p == '"') {
                    p++;
                    while (*p && *p != '"') {
                        unsigned char ch = (unsigned char)*p; /* C89: declare at block start */
                        /* Enforce printable ASCII range 32..126 inclusive */
                        if (ch < 32 || ch > 126) {
                            error_report_ex(ERR_SEV_ERROR, ERR_STRING_BAD_CHAR, NULL, dd->src_line, NULL);
                            return;
                        }
                        if ((data_base_addr + DC) >= MEMORY_SIZE) {
                            error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, dd->src_line, ".string exceeds memory size");
                            return;
                        }
                        data_image[base_dc].value = (unsigned short)ch;
                        data_image[base_dc].src_line = dd->src_line;
                        base_dc++;
                        DC++;
                        data_count++;
                        p++;
                    }
                    /* null terminator */
                    if ((data_base_addr + DC) >= MEMORY_SIZE) {
                        error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, dd->src_line, ".string exceeds memory size");
                        return;
                    }
                    data_image[base_dc].value = 0;
                    data_image[base_dc].src_line = dd->src_line;
                    base_dc++;
                    DC++;
                    data_count++;
                } else {
                    error_report_ex(ERR_SEV_ERROR, ERR_STRING_NOT_QUOTED, NULL, dd->src_line, NULL);
                    return;
                }
            }
        } else if (strcmp(dd->type, ".mat") == 0) {
            int rows = 0, cols = 0, total = 0;
            if (dd->label[0]) {
                /* mark as data, not extern */
                insert_label(&label_table_head, dd->label, data_base_addr + DC, 0, 1, 0);
                base_dc = DC;
            }
            if (!parse_dims(dd->operands, &rows, &cols)) {
                error_report_ex(ERR_SEV_ERROR, ERR_MAT_SIZE_INVALID, NULL, dd->src_line, NULL);
                return;
            }
            total = rows * cols;
            {
                /* Move p to just after the SECOND closing ']' */
                const char *p = dd->operands;
                int closes = 0;
                while (*p) {
                    if (*p == ']') {
                        closes++;
                        if (closes == 2) { p++; break; }
                    }
                    p++;
                }
                /* Skip whitespace and an optional comma */
                while (*p && isspace((unsigned char)*p)) p++;
                if (*p == ',') { p++; }
                while (*p && isspace((unsigned char)*p)) p++;

                /* Parse up to total initializers; fill remaining with 0 */
                {
                    int filled = 0;
                    while (*p && filled < total) {
                        while (*p && isspace((unsigned char)*p)) p++;
                        if (!*p) break;
                        if (*p == ',') { p++; continue; }
                        {
                            char *end;
                            long val = strtol(p, &end, 10);
                            if (p == end) break; /* no more numbers */
                            if ((data_base_addr + DC) >= MEMORY_SIZE) {
                                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, dd->src_line, ".mat exceeds memory size");
                                return;
                            }
                            data_image[base_dc + filled].value = (unsigned short)val;
                            data_image[base_dc + filled].src_line = dd->src_line;
                            filled++;
                            DC++;
                            data_count++;
                            p = end;
                            while (*p && isspace((unsigned char)*p)) p++;
                            if (*p == ',') p++;
                        }
                    }
                    while (filled < total) {
                        if ((data_base_addr + DC) >= MEMORY_SIZE) {
                            error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, dd->src_line, ".mat exceeds memory size");
                            return;
                        }
                        data_image[base_dc + filled].value = 0;
                        data_image[base_dc + filled].src_line = dd->src_line;
                        filled++;
                        DC++;
                        data_count++;
                    }
                    /* Warn on extra values */
                    while (*p) {
                        while (*p && isspace((unsigned char)*p)) p++;
                        if (*p == ',') { p++; continue; }
                        if (!*p) break;
                        {
                            char *end2; (void)strtol(p, &end2, 10);
                            if (p != end2) {
                                error_report_ex(ERR_SEV_WARNING, ERR_MAT_INIT_COUNT_MISMATCH, NULL, dd->src_line, "extra initializers ignored");
                                break;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    *data_image_ptr = data_image;
    *data_count_ptr = data_count;
    *DC_ptr = DC;
    *label_table_head_ptr = label_table_head;
}
