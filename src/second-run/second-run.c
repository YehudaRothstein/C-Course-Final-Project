#include <stdio.h>
#include <string.h>
#include "second-run.h"
#include "label_table.h"
#include "other_table.h"
#include "code_conversion.h"
#include "../utils/base4.h"
#include "../utils/utils.h"

/* Mask for the 10-bit word payload with A/R/E (low 2 bits) cleared */
#define PAYLOAD_MASK_10B 0x3FCu

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

/* Build path into dst safely (C90): dst gets dir + optional '/' + base + suffix, truncated if needed */
static void build_path_safe(char *dst, size_t dst_sz, const char *dir, const char *base, const char *suffix) {
    size_t len_dir = strlen(dir);
    int need_sep = (len_dir > 0 && dir[len_dir - 1] != '/') ? 1 : 0;
    size_t len_sep = need_sep ? 1u : 0u;
    size_t len_base = strlen(base);
    size_t len_suf = strlen(suffix);
    size_t pos = 0;

    if (dst_sz == 0) return;

    /* Calculate max base part to fit into dst */
    if (len_dir + len_sep + len_base + len_suf >= dst_sz) {
        size_t max_base;
        if (dst_sz <= 1) {
            dst[0] = '\0';
            return;
        }
        if (len_dir >= dst_sz - 1) {
            len_dir = dst_sz - 1; /* we'll truncate dir too */
            len_sep = 0;
            len_base = 0;
            len_suf = 0;
        } else {
            max_base = (dst_sz - 1) - len_dir - len_sep - len_suf;
            if (len_base > max_base) len_base = max_base;
        }
    }

    /* Copy dir */
    if (len_dir > 0) { memcpy(dst + pos, dir, len_dir); pos += len_dir; }
    if (need_sep && pos < dst_sz - 1) { dst[pos++] = '/'; }
    /* Copy base */
    if (len_base > 0 && pos < dst_sz - 1) {
        size_t can_copy = dst_sz - 1 - pos;
        if (len_base > can_copy) len_base = can_copy;
        memcpy(dst + pos, base, len_base); pos += len_base;
    }
    /* Copy suffix */
    if (len_suf > 0 && pos < dst_sz - 1) {
        size_t can_copy_s = dst_sz - 1 - pos;
        if (len_suf > can_copy_s) len_suf = can_copy_s;
        memcpy(dst + pos, suffix, len_suf); pos += len_suf;
    }

    dst[pos] = '\0';
}

/* Handle an external reference: clear A/R/E payload, set ARE=2, and write to .ext */
static void handle_extern_reference(
    code_conv_t *code_word,
    const char *symbol,
    FILE **ext_file,
    const char *ext_path,
    int word_abs_addr
) {
    /* keep payload, clear low two bits */
    code_word->value &= PAYLOAD_MASK_10B;
    code_word->are = 2; /* E */
    if (!*ext_file) {
        *ext_file = fopen(ext_path, "w");
    }
    if (*ext_file) {
        char addr4[5];
        fmt_addr4(word_abs_addr, addr4);
        fprintf(*ext_file, "%s %s\n", symbol, addr4);
    }
}

/* מעבד הפניות פנימיות מקודד את הכתובת ומגדיר את ARE=1 */
static void handle_internal_reference(code_conv_t *code_word, unsigned short label_addr) {
    code_word->value = (unsigned short)(((unsigned short)label_addr << 2) & PAYLOAD_MASK_10B);
    code_word->are = 1; /* R */
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
    char output_dir[512];
    int has_entry_symbols = 0;
    int word_index;

    /* מבטיח שהתקייה לבניית  הקבצים קיימת */
    ensure_output_dir(base_filename, output_dir, sizeof(output_dir));

    /* בונה שמות קבצי פלט */
    {
        const char *dir_path = (output_dir[0] != '\0') ? output_dir : ".";
        build_path_safe(ext_path, sizeof(ext_path), dir_path, base_filename, ".ext");
        build_path_safe(ent_path, sizeof(ent_path), dir_path, base_filename, ".ent");
    }

    /* בודק אם יש צורך בקובץ .ent */
    for (word_index = 0; word_index < entries_count; word_index++) {
        LabelNode *label = find_label(label_table, entries[word_index].name);
        if (label && !label->is_extern) has_entry_symbols = 1;
    }
    /* אם יש צורך בקובץ .ent, פותחים אותו לכתיבה */
    if (has_entry_symbols) ent_file = fopen(ent_path, "w");

    /* כותב את הקוד המתקדם לקובץ הפלט */
    for (word_index = 0; word_index < code_count; word_index++) {
        const char *symbol_name;
        LabelNode *label;

        if (code[word_index].are != 1) continue; /* רק עבור מקומות עם ARE 1 */

        symbol_name = code[word_index].ext_name;
        if (!symbol_name || !*symbol_name) continue;

        /* חיפוש תווית */
        label = find_label(label_table, symbol_name);
        /* אם התווית נמצאה */
        if (!label) continue;

        /* טיפול בהפניות חיצוניות ופנימיות */
        if (label->is_extern) {
            handle_extern_reference(&code[word_index], symbol_name, &ext_file, ext_path, 100 + word_index);
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
            }
        }
        /* סוגר את קובץ ה-.ent */
        fclose(ent_file);
    }
    /* סוגר את קובץ ה-.ext */
    if (ext_file)
        fclose(ext_file);
}
