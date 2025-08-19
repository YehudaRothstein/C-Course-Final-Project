#include <stdio.h>
#include <string.h>
#include "output/output_writer.h"
#include "code_conversion.h"
#include "../../error-handler/error-handler.h"
#include "../../utils/utils.h"

void write_code_file(const char *out_filename, code_conv_t *code, int code_count, data_word *data_image, int data_count) {
    char ob_filename[300];
    char base_name[256];
    int i;
    int d;

    /* אם קיימות שגיאות קודמות, אין לייצר קובץ פלט */
    if (error_get_error_count() > 0) {
        return;
    }

    get_basefile(out_filename, base_name, sizeof(base_name));
    {
        size_t cap = sizeof(ob_filename);
        ob_filename[0] = '\0';
        strncat(ob_filename, base_name, cap - 1);
        strncat(ob_filename, ".ob", cap - 1 - strlen(ob_filename));
    }

    {
        FILE *fp = fopen(ob_filename, "w");
        if (!fp) {
            error_report_ex(ERR_SEV_ERROR, ERR_OUTPUT_OB_WRITE_FAIL, ob_filename, 0, NULL);
            return;
        }
        /* Header: lengths (code_count, data_count) in special base-4 (trim leading 'a') */
        {
            char code_count_base4[6], data_count_base4[6];
            for (d = 4; d >= 0; d--) {
                int digit = ((unsigned int)code_count >> (d * 2)) & 0x3;
                code_count_base4[4 - d] = "abcd"[digit];
            }
            code_count_base4[5] = '\0';
            for (d = 4; d >= 0; d--) {
                int digit2 = ((unsigned int)data_count >> (d * 2)) & 0x3;
                data_count_base4[4 - d] = "abcd"[digit2];
            }
            data_count_base4[5] = '\0';
            {
                char *cc_ptr = code_count_base4;
                char *dc_ptr = data_count_base4;
                while (*cc_ptr == 'a' && *(cc_ptr+1)) cc_ptr++;
                while (*dc_ptr == 'a' && *(dc_ptr+1)) dc_ptr++;
                fprintf(fp, "%s %s\n", cc_ptr, dc_ptr);
            }
        }
        for (i = 0; i < code_count; i++) {
            int addr = 100 + i;
            unsigned short are_bits = (code[i].are == 0 ? 0 : (code[i].are == 1 ? 2 : 1));
            unsigned short code_val = (code[i].value & 0x3FC) | are_bits;
            char code_base4[6];
            char addr_base4[5];
            /* address in base-4 (4 digits) */
            {
                int n = addr;
                for (d = 3; d >= 0; d--) {
                    int ad = n % 4;
                    addr_base4[d] = "abcd"[ad];
                    n /= 4;
                }
                addr_base4[4] = '\0';
            }
            for (d = 4; d >= 0; d--) {
                int digit = (code_val >> (d * 2)) & 0x3;
                code_base4[4 - d] = "abcd"[digit];
            }
            code_base4[5] = '\0';
            fprintf(fp, "%s %s\n", addr_base4, code_base4);
        }
        for (i = 0; i < data_count; i++) {
            int addr = 100 + code_count + i;
            unsigned short data_val = data_image[i].value & 0x3FF;
            char data_base4[6];
            char addr_base4[5];
            /* address in base-4 (4 digits) */
            {
                int n2 = addr;
                for (d = 3; d >= 0; d--) {
                    int ad2 = n2 % 4;
                    addr_base4[d] = "abcd"[ad2];
                    n2 /= 4;
                }
                addr_base4[4] = '\0';
            }
            for (d = 4; d >= 0; d--) {
                int digit = (data_val >> (d * 2)) & 0x3;
                data_base4[4 - d] = "abcd"[digit];
            }
            data_base4[5] = '\0';
            fprintf(fp, "%s %s\n", addr_base4, data_base4);
        }
        fclose(fp);
    }
}
