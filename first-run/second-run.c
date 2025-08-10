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
    snprintf(extname, sizeof(extname), "outputs/%s.ext", base_filename);
    FILE *entf = NULL;
    char entname[256];
    snprintf(entname, sizeof(entname), "outputs/%s.ent", base_filename);

    int ent_needed = 0;
    for (int i = 0; i < entries_count; i++) {
        LabelNode *lbl = find_label(label_table, entries[i].name);
        if (lbl && !lbl->is_extern) ent_needed = 1;
    }
    if (ent_needed) entf = fopen(entname, "w");

    // Resolve relocatable words using stored symbol names
    for (int i = 0; i < code_count; i++) {
        if (code[i].are == 1) {
            const char *sym = code[i].ext_name;
            if (sym && *sym) {
                LabelNode *lbl = find_label(label_table, sym);
                if (lbl) {
                    if (lbl->is_extern) {
                        // External symbol: value stays 0, ARE=E
                        code[i].value &= 0x3FC;
                        code[i].are = 2; // mark as external
                        if (!extf) extf = fopen(extname, "w");
                        if (extf) fprintf(extf, "%s %d\n", sym, 100 + i);
                    } else {
                        // Internal label: address + offset for data labels
                        int addr = lbl->address + (lbl->is_data ? code_count : 0);
                        code[i].value = ((unsigned short)addr & 0x3FF);
                        code[i].are = 1; // relocatable
                    }
                }
            }
        }
    }

    if (entf) {
        for (int i = 0; i < entries_count; i++) {
            LabelNode *lbl = find_label(label_table, entries[i].name);
            if (lbl && !lbl->is_extern) {
                int addr = lbl->address + (lbl->is_data ? code_count : 0);
                fprintf(entf, "%s %d\n", lbl->name, addr);
            }
        }
        fclose(entf);
    }
    if (extf) fclose(extf);
}
