#include <stdio.h>
#include <string.h>
#include "second-run.h"
#include "label_table.h"
#include "other_table.h"
#include "code_conversion.h"
#include "../utils/base4.h"

void exe_second_pass(
    code_conv *code, int code_count,
    LabelNode *label_table,
    other_table *entries, int entries_count,
    other_table *externs, int externs_count,
    const char *base_filename
) {
    FILE *extf = NULL;
    char extname[256];
    FILE *entf = NULL;
    char entname[256];
    int ent_needed = 0;
    int i;

    sprintf(extname, "outputs/%s.ext", base_filename);
    sprintf(entname, "outputs/%s.ent", base_filename);

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
                        /* Encode address in bits 2-9 (shift left 2) leaving ARE in bits 0-1 */
                        code[i].value = (unsigned short)(((unsigned short)lbl->address << 2) & 0x3FC);
                        code[i].are = 1; 
                    }
                }
            }
        }
    }

    if (entf) {
        for (i = 0; i < entries_count; i++) {
            LabelNode *lbl = find_label(label_table, entries[i].name);
            if (lbl && !lbl->is_extern) {
                fprintf(entf, "%s %d\n", lbl->name, lbl->address);
            }
        }
        fclose(entf);
    }
    if (extf) fclose(extf);
}
