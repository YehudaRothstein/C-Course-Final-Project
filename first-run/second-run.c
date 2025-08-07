#include <stdio.h>
#include <string.h>
#include "second-run.h"
#include "label_table.h"
#include "other_table.h"
#include "code_conversion.h"

// Helper: convert a 10-bit value to the special base-4 string (5 chars, a-d)
void to_special_base4_str(unsigned short value, char *out);

void exe_second_pass(
    code_conv *code, int code_count,
    LabelNode *label_table,
    other_table *entries, int entries_count,
    other_table *externs, int externs_count,
    const char *base_filename
) {
    // For .ext output
    FILE *extf = NULL;
    char extname[256];
    snprintf(extname, sizeof(extname), "outputs/%s.ext", base_filename);
    // For .ent output
    FILE *entf = NULL;
    char entname[256];
    snprintf(entname, sizeof(entname), "outputs/%s.ent", base_filename);
    int ent_needed = 0;
    // Open .ent only if needed
    for (int i = 0; i < entries_count; i++) {
        LabelNode *lbl = find_label(label_table, entries[i].name);
        if (lbl && !lbl->is_extern) ent_needed = 1;
    }
    if (ent_needed) entf = fopen(entname, "w");
    // For each code word with ARE=1 (relocatable), resolve label
    for (int i = 0; i < code_count; i++) {
        if (code[i].are == 1) {
            int found = 0;
            // Try externs first
            for (int j = 0; j < externs_count; j++) {
                LabelNode *lbl = find_label(label_table, externs[j].name);
                if (lbl && lbl->is_extern) {
                    // Mark as external
                    code[i].are = 2;
                    code[i].value = 0; // Value for external is 0
                    if (!extf) extf = fopen(extname, "w");
                    if (extf) {
                        char addr[6];
                        to_special_base4_str((unsigned short)(100 + i), addr);
                        fprintf(extf, "%s %s\n", externs[j].name, addr);
                    }
                    found = 1;
                    break;
                }
            }
            if (!found) {
                // Try to find a label in the label table (not extern)
                LabelNode *lbl = label_table;
                while (lbl) {
                    if (!lbl->is_extern && lbl->address >= 0) {
                        code[i].value = (unsigned short)(lbl->address + (lbl->is_data ? code_count : 0));
                        code[i].are = 1;
                        break;
                    }
                    lbl = lbl->next;
                }
            }
        }
    }
    // Write .ent file
    if (entf) {
        for (int i = 0; i < entries_count; i++) {
            LabelNode *lbl = find_label(label_table, entries[i].name);
            if (lbl && !lbl->is_extern) {
                char addr[6];
                to_special_base4_str((unsigned short)(lbl->address + (lbl->is_data ? code_count : 0)), addr);
                fprintf(entf, "%s %s\n", lbl->name, addr);
            }
        }
        fclose(entf);
    }
    if (extf) fclose(extf);
}
