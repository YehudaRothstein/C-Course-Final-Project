#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "process_data_directives.h"
#include "memory_map_data.h"
#include "memory_map.h"

void process_data_directives(
    DataDirective *data_directives,
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
