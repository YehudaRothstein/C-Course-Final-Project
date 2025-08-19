#include <stdio.h>
#include <string.h>
#include "second-run.h"
#include "label_table.h"
#include "other_table.h"
#include "code_conversion.h"
#include "../utils/utils.h"
#include "../error-handler/error-handler.h"

/* עוזר לעצב כתובת מיוחדת בבסיס 4 באורך 4 תווים */
static void fmt_addr4(int addr, char out[5]) {
    int d;
    for (d = 3; d >= 0; d--) {
        int ad = addr % 4;
        out[d] = "abcd"[ad];
        addr /= 4;
    }
    out[4] = '\0';
}

/* טיפול בהפניה חיצונית */
static void handle_extern_reference(
    code_conv_t *code_word,
    const char *symbol,
    FILE **ext_file,
    const char *ext_path,
    int word_abs_addr
) {

    code_word->value = (unsigned short)((code_word->value / 4) * 4); /* שמירה על התוכן, ניקוי שני הביטים הנמוכים */
    code_word->are = 2; /* E */
    if (!*ext_file) {
        *ext_file = fopen(ext_path, "w");
    }
    /* כתיבה לקובץ החיצוני */
    if (*ext_file) {
        char addr4[5];
        fmt_addr4(word_abs_addr, addr4);
        fprintf(*ext_file, "%s %s\n", symbol, addr4);
    }
}

/* מעבד הפניות פנימיות מקודד את הכתובת ומגדיר את ARE=1 */
static void handle_internal_reference(code_conv_t *code_word, unsigned short label_addr) {
    /* שמים את הכתובת בביטים 2..9; השיפט מבטיח ששני הביטים הנמוכים הם 0 */
    code_word->value = (unsigned short)(((unsigned short)label_addr) << 2);
    code_word->are = 1; /* Relocatalbe */
}

/* הפעלת המעבר השני */
void exe_second_pass(
    code_conv_t *code, int code_count,
    LabelNode *label_table, other_table *entries, int entries_count,
    other_table *externs, int externs_count, const char *base_filename ) {
    FILE *ext_file = NULL;
    char ext_path[512];
    FILE *ent_file = NULL;
    char ent_path[512];
    char base_only[256];
    int has_entry_symbols = 0;
    int word_index;
    int errs_at_start = error_get_error_count();

    /* בונה שמות קבצי פלט בצורה פשוטה: <base>.ext / <base>.ent בתיקייה הנוכחית */
    get_basefile(base_filename, base_only, sizeof(base_only));
    {
        size_t blen = strlen(base_only);
        size_t i;
        /* ext path */
        if (blen > sizeof(ext_path) - 5) blen = sizeof(ext_path) - 5; /* מקום עבור .ext + NUL */
        for (i = 0; i < blen; i++) ext_path[i] = base_only[i];
        ext_path[blen] = '\0';
        strcat(ext_path, ".ext");
        /* ent path */
        blen = strlen(base_only);
        if (blen > sizeof(ent_path) - 5) blen = sizeof(ent_path) - 5; /* מקום עבור .ent + NUL */
        for (i = 0; i < blen; i++) ent_path[i] = base_only[i];
        ent_path[blen] = '\0';
        strcat(ent_path, ".ent");
    }

    /* בודק אם יש צורך בקובץ .ent */
    for (word_index = 0; word_index < entries_count; word_index++) {
        LabelNode *label = find_label(label_table, entries[word_index].name);
        if (label && !label->is_extern) has_entry_symbols = 1;
    }
    /* אם יש צורך בקובץ .ent ואין שגיאות קודמות – פותחים אותו לכתיבה */
    if (has_entry_symbols && errs_at_start == 0) ent_file = fopen(ent_path, "w");

    /* כותב את הקוד המתקדם לקובץ הפלט */
    for (word_index = 0; word_index < code_count; word_index++) {
        const char *symbol_name;
        LabelNode *label;

        if (code[word_index].are != 1) continue; /* רק עבור מקומות עם ARE 1 */

        symbol_name = code[word_index].ext_name;
        if (!symbol_name || !*symbol_name) continue;

        /* חיפוש תווית */
        label = find_label(label_table, symbol_name);
        /* אם התווית לא נמצאה – זוהי שגיאה סמנטית (סמל לא מוגדר) */
        if (!label) {
            error_report_ex(ERR_SEV_ERROR, ERR_LABEL_UNDEFINED, base_filename, code[word_index].source_line_num, symbol_name);
            continue;
        }

        /* טיפול בהפניות חיצוניות ופנימיות */
        if (label->is_extern) {
            /* כתיבה ל-.ext רק אם לא היו שגיאות בתחילת המעבר */
            if (errs_at_start == 0) {
                handle_extern_reference(&code[word_index], symbol_name, &ext_file, ext_path, 100 + word_index);
            }
        } else {
            handle_internal_reference(&code[word_index], (unsigned short)label->address);
        }
    }

    /* כותב את הקוד לקובץ הפלט */
    if (ent_file) {
        int entry_index;
        /* עוברים על כל הכניסות */
        for (entry_index = 0; entry_index < entries_count; entry_index++) {
            /* חיפוש תווית */
            LabelNode *entry_label = find_label(label_table, entries[entry_index].name);
            /* אם התווית נמצאה */
            if (entry_label && !entry_label->is_extern) {
                char addr4[5];
                fmt_addr4(entry_label->address, addr4);
                fprintf(ent_file, "%s %s\n", entry_label->name, addr4);
            } else if (!entry_label) {
                /* .entry על סמל שלא הוגדר בקובץ – שגיאה */
                error_report_ex(ERR_SEV_ERROR, ERR_ENTRY_UNDEFINED, base_filename, 0, entries[entry_index].name);
            }
        }
        /* סוגר את קובץ ה-.ent */
        fclose(ent_file);
    }
    /* סוגר את קובץ ה-.ext */
    if (ext_file) {
        fclose(ext_file);
    }

    /*
     * הדפסה אינפורמטיבית על סיום המעבר השני. אין בה כדי להצביע על יצירת קבצים.
     */
    printf("Second pass completed for %s\n", base_filename);
}
