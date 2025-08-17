#include <stdio.h>
#include "print/memory_map_print.h"
#include "code_conversion.h"
#include "data_word.h"
#include "structures/label_table.h"

void print_memory_map(int code_count, code_conv_t *code, int data_count, data_word *data_image, LabelNode *label_table_head) {
    int IC_INIT_VALUE = 100;
    int i;
    for (i = 0; i < code_count; ++i) {
        int addr = IC_INIT_VALUE + i;
        /* Compose value exactly like the .ob writer (include ARE in low 2 bits) */
        {
            unsigned short are_bits = (code[i].are == 0 ? 0 : (code[i].are == 1 ? 2 : 1));
            unsigned short val = (code[i].value & 0x3FC) | are_bits;
            char val_base4[6];
            int d;
            for (d = 4; d >= 0; d--) {
                int digit = (val >> (d * 2)) & 0x3;
                val_base4[4 - d] = "abcd"[digit];
            }
            val_base4[5] = '\0';
            {
                const char *label = NULL;
                LabelNode *curr = label_table_head;
                while (curr) {
                    if (curr->is_code && curr->address == addr) { label = curr->name; break; }
                    curr = curr->next;
                }
                if (label)
                    printf("%-10d %-8s %-10d %-8s  ; %s\n", addr, "CODE", val, val_base4, label);
                else
                    printf("%-10d %-8s %-10d %-8s\n", addr, "CODE", val, val_base4);
            }
        }
    }
    for (i = 0; i < data_count; ++i) {
        int addr = IC_INIT_VALUE + code_count + i;
        unsigned short val = data_image[i].value & 0x3FF;
        char val_base4[6];
        int d2;
        for (d2 = 4; d2 >= 0; d2--) {
            int digit2 = (val >> (d2 * 2)) & 0x3;
            val_base4[4 - d2] = "abcd"[digit2];
        }
        val_base4[5] = '\0';
        printf("%-10d %-8s %-10d %-8s\n", addr, "DATA", val, val_base4);
    }
}
