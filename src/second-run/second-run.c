#include <stdio.h>
#include <string.h>
#include "second-run.h"
#include "label_table.h"
#include "other_table.h"
#include "code_conversion.h"
#include "../utils/base4.h"
#include "../utils/utils.h"

void exe_second_pass(
    code_conv_t *code, int code_count,
    LabelNode *label_table,
    other_table *entries, int entries_count,
    other_table *externs, int externs_count,
    const char *base_filename
) {
    FILE *extf = NULL;
    char extname[512];
    FILE *entf = NULL;
    char entname[512];
    char out_dir[512];
    int ent_needed = 0;
    int i;

    /* Ensure per-file output directory exists */
    ensure_output_dir(base_filename, out_dir, sizeof(out_dir));{
        size_t pos = 0, j;
        size_t cap = sizeof(extname);
        for (j = 0; out_dir[j] && pos + 1 < cap; j++) extname[pos++] = out_dir[j];
#if defined(_WIN32) || defined(_WIN64)
        if (pos + 1 < cap) extname[pos++] = '\\';
#else
        if (pos + 1 < cap) extname[pos++] = '/';
#endif
        for (j = 0; base_filename[j] && pos + 1 < cap; j++) extname[pos++] = base_filename[j];
        {
            const char *suf = ".ext";
            for (j = 0; suf[j] && pos + 1 < cap; j++) extname[pos++] = suf[j];
        }
        extname[pos] = '\0';
    }
    {
        size_t pos2 = 0, k;
        size_t cap2 = sizeof(entname);
        for (k = 0; out_dir[k] && pos2 + 1 < cap2; k++) entname[pos2++] = out_dir[k];
#if defined(_WIN32) || defined(_WIN64)
        if (pos2 + 1 < cap2) entname[pos2++] = '\\';
#else
        if (pos2 + 1 < cap2) entname[pos2++] = '/';
#endif
        for (k = 0; base_filename[k] && pos2 + 1 < cap2; k++) entname[pos2++] = base_filename[k];
        {
            const char *suf2 = ".ent";
            for (k = 0; suf2[k] && pos2 + 1 < cap2; k++) entname[pos2++] = suf2[k];
        }
        entname[pos2] = '\0';
    }

    for (i = 0; i < entries_count; i++) {
        LabelNode *lbl = find_label(label_table, entries[i].name);
        if (lbl && !lbl->is_extern) ent_needed = 1;
    }
    if (ent_needed) entf = fopen(entname, "w");

    for (i = 0; i < code_count; i++) {
        if (code[i].are == 1) {
            const char *sym = code[i].ext_name;
            if (sym && *sym) {
                LabelNode *lbl = find_label(label_table, sym);
                if (lbl) {
                    if (lbl->is_extern) {
                        code[i].value &= 0x3FC;
                        code[i].are = 2; 
                        if (!extf) extf = fopen(extname, "w");
                        if (extf) fprintf(extf, "%s %d\n", sym, 100 + i);
                    } else {
                        code[i].value = (unsigned short)(((unsigned short)lbl->address << 2) & 0x3FC);
                        code[i].are = 1; 
                    }
                }
            }
        }
    }

    if (entf) {
        int j;
        for (j = 0; j < entries_count; j++) {
            LabelNode *lbl2 = find_label(label_table, entries[j].name);
            if (lbl2 && !lbl2->is_extern) {
                fprintf(entf, "%s %d\n", lbl2->name, lbl2->address);
            }
        }
        fclose(entf);
    }
    if (extf) fclose(extf);
}
