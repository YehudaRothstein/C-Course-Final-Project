#include <stdio.h>
#include <string.h>
#include "output/output_writer.h"
#include "../../error-handler/error-handler.h"

void write_code_file(const char *out_filename, code_conv *code, int code_count, data_word *data_image, int data_count) {
    char ob_filename[300];
    const char *slash = strrchr(out_filename, '/');
    const char *base = slash ? slash + 1 : out_filename;
    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    char base_name[256];
    int i;
    int d;

    strncpy(base_name, base, len);
    base_name[len] = '\0';

    sprintf(ob_filename, "outputs/%s.ob", base_name);
    {
        FILE *fp = fopen(ob_filename, "w");
        if (!fp) {
            error_report_ex(ERR_SEV_ERROR, ERR_OUTPUT_OB_WRITE_FAIL, ob_filename, 0, NULL);
            return;
        }
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
            for (d = 4; d >= 0; d--) {
                int digit = (code_val >> (d * 2)) & 0x3;
                code_base4[4 - d] = "abcd"[digit];
            }
            code_base4[5] = '\0';
            fprintf(fp, "%d %s\n", addr, code_base4);
        }
        for (i = 0; i < data_count; i++) {
            int addr = 100 + code_count + i;
            unsigned short data_val = data_image[i].value & 0x3FF;
            char data_base4[6];
            for (d = 4; d >= 0; d--) {
                int digit = (data_val >> (d * 2)) & 0x3;
                data_base4[4 - d] = "abcd"[digit];
            }
            data_base4[5] = '\0';
            fprintf(fp, "%d %s\n", addr, data_base4);
        }
        fclose(fp);
    }
    {
        char dec_filename[300];
        FILE *fdec;
        sprintf(dec_filename, "outputs/%s.bin", base_name);
        fdec = fopen(dec_filename, "w");
        if (!fdec) { error_report_ex(ERR_SEV_ERROR, ERR_OUTPUT_BIN_WRITE_FAIL, dec_filename, 0, NULL); return; }
        for (i = 0; i < code_count; i++) {
            int addr = 100 + i;
            unsigned short are_bits = (code[i].are == 0 ? 0 : (code[i].are == 1 ? 2 : 1));
            unsigned short val = (code[i].value & 0x3FC) | are_bits;
            char bits[11];
            int b;
            for (b = 9; b >= 0; b--) bits[9 - b] = ((val >> b) & 1) ? '1' : '0';
            bits[10] = '\0';
            fprintf(fdec, "%d %s\n", addr, bits);
        }
        for (i = 0; i < data_count; i++) {
            int addr = 100 + code_count + i;
            unsigned short val = data_image[i].value & 0x3FF;
            char bits[11];
            int b2;
            for (b2 = 9; b2 >= 0; b2--) bits[9 - b2] = ((val >> b2) & 1) ? '1' : '0';
            bits[10] = '\0';
            fprintf(fdec, "%d %s\n", addr, bits);
        }
        fclose(fdec);
    }
}
