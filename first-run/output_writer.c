#include <stdio.h>
#include <string.h>
#include "output_writer.h"

void write_code_file(const char *out_filename, code_conv *code, int code_count, data_word *data_image, int data_count) {
    char ob_filename[300];
    const char *slash = strrchr(out_filename, '/');
    const char *base = slash ? slash + 1 : out_filename;
    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    char base_name[256];
    strncpy(base_name, base, len);
    base_name[len] = '\0';

    // Write the .ob file (addresses in decimal on the left column)
    snprintf(ob_filename, sizeof(ob_filename), "outputs/%s.ob", base_name);
    FILE *fp = fopen(ob_filename, "w");
    if (!fp) return;

    // Header: keep counts in special base-4 as before
    char code_count_base4[6], data_count_base4[6];
    for (int d = 4; d >= 0; d--) {
        int digit = ((unsigned int)code_count >> (d * 2)) & 0x3;
        code_count_base4[4 - d] = "abcd"[digit];
    }
    code_count_base4[5] = '\0';
    for (int d = 4; d >= 0; d--) {
        int digit = ((unsigned int)data_count >> (d * 2)) & 0x3;
        data_count_base4[4 - d] = "abcd"[digit];
    }
    data_count_base4[5] = '\0';
    char *cc_ptr = code_count_base4;
    char *dc_ptr = data_count_base4;
    while (*cc_ptr == 'a' && *(cc_ptr+1)) cc_ptr++;
    while (*dc_ptr == 'a' && *(dc_ptr+1)) dc_ptr++;
    fprintf(fp, "%s %s\n", cc_ptr, dc_ptr);

    // Body: addresses in decimal, values in special base-4 on the right
    for (int i = 0; i < code_count; i++) {
        int addr = 100 + i;
        // Combine the word value (bits 9..2) with ARE bits (bits 1..0)
        unsigned short are_bits = (code[i].are == 0 ? 0 : (code[i].are == 1 ? 2 : 1));
        unsigned short code_val = (code[i].value & 0x3FC) | are_bits;
        char code_base4[6];
        for (int d = 4; d >= 0; d--) {
            int digit = (code_val >> (d * 2)) & 0x3;
            code_base4[4 - d] = "abcd"[digit];
        }
        code_base4[5] = '\0';
        fprintf(fp, "%d %s\n", addr, code_base4);
    }
    for (int i = 0; i < data_count; i++) {
        int addr = 100 + code_count + i;
        unsigned short data_val = data_image[i].value & 0x3FF; // data has no ARE, value is full 10 bits of content
        char data_base4[6];
        for (int d = 4; d >= 0; d--) {
            int digit = (data_val >> (d * 2)) & 0x3;
            data_base4[4 - d] = "abcd"[digit];
        }
        data_base4[5] = '\0';
        fprintf(fp, "%d %s\n", addr, data_base4);
    }
    fclose(fp);

    // Additionally, auto-generate a decoded file with 10-bit binary (0/1) values
    char dec_filename[300];
    snprintf(dec_filename, sizeof(dec_filename), "outputs/%s.bin", base_name);
    FILE *fdec = fopen(dec_filename, "w");
    if (!fdec) return;

    // One line per memory word: decimal address and 10-bit binary value
    for (int i = 0; i < code_count; i++) {
        int addr = 100 + i;
        unsigned short are_bits = (code[i].are == 0 ? 0 : (code[i].are == 1 ? 2 : 1));
        unsigned short val = (code[i].value & 0x3FC) | are_bits;
        char bits[11];
        for (int b = 9; b >= 0; b--) bits[9 - b] = ((val >> b) & 1) ? '1' : '0';
        bits[10] = '\0';
        fprintf(fdec, "%d %s\n", addr, bits);
    }
    for (int i = 0; i < data_count; i++) {
        int addr = 100 + code_count + i;
        unsigned short val = data_image[i].value & 0x3FF;
        char bits[11];
        for (int b = 9; b >= 0; b--) bits[9 - b] = ((val >> b) & 1) ? '1' : '0';
        bits[10] = '\0';
        fprintf(fdec, "%d %s\n", addr, bits);
    }
    fclose(fdec);
}
