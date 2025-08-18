#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pre-assembler/pre-assembler.h"
#include "first-run/first-run.h"
#include "utils/utils.h"

/* לקחת את שם הקובץ ללא הסיומת */
static void basename_no_ext(const char *path, char *out, size_t outSize) {
    /* מוצאים את הסלאש האחרון */
    const char *slash = strrchr(path, '/');
    /* אם אין סלאש, אז זה שם קובץ ישיר */
    const char *base = slash ? slash + 1 : path;
    /* מוצאים את הנקודה האחרונה */
    const char *dot = strrchr(base, '.');
    /* אם יש נקודה, חותכים שם עד הנקודה */
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    /* אם האורך גדול מדי, חותכים אותו */
    if (len >= outSize) len = outSize - 1;
    /* מעתיקים את הבסיס לתוך התוצאה */
    strncpy(out, base, len);
    out[len] = '\0';
}

int main(int argc, char *argv[]) {
    int i;
    int any_fail = 0;

    if (argc < 2) {
        printf("Usage: %s <file1> [file2 ...]\n", argv[0]);
        return 1;
    }

    /* עוברים על כל קובץ בנפרד */
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        char input_path[512];
        char base[256];
        char out_dir[512];
        char macro_spread_file[512];
        char macro_log_path[512];
        int pre_assembler_result;
        int first_pass_res;

        {
            size_t cap = sizeof(input_path);
            size_t inlen = strlen(arg);
            /* מעתיקים את הנתיב  */
            if (inlen >= cap) inlen = cap - 1;
            memcpy(input_path, arg, inlen);
            input_path[inlen] = '\0';
        }

        basename_no_ext(input_path, base, sizeof(base));

        /* בונים את נתיב הפלט */
        if (!ensure_output_dir(base, out_dir, sizeof(out_dir))) {
            printf("Failed to have output directory for %s\n", base);
            any_fail = 1;
            continue;
        }

        /* מכינים את ה קובץ  של המקרו */
        {
            size_t cap = sizeof(macro_log_path);
            size_t pos = 0;
            size_t j;
            for (j = 0; out_dir[j] && pos + 1 < cap; j++) macro_log_path[pos++] = out_dir[j];
            if (pos + 1 < cap) macro_log_path[pos++] = '/';
            for (j = 0; base[j] && pos + 1 < cap; j++) macro_log_path[pos++] = base[j];
            {
                const char *suffix = ".macros.txt";
                for (j = 0; suffix[j] && pos + 1 < cap; j++) macro_log_path[pos++] = suffix[j];
            }
            macro_log_path[pos] = '\0';
        }

        /* בונים את קובץ הפלט של המקרו */
        {
            size_t cap2 = sizeof(macro_spread_file);
            size_t pos2 = 0;
            size_t k;
            for (k = 0; out_dir[k] && pos2 + 1 < cap2; k++) macro_spread_file[pos2++] = out_dir[k];
            if (pos2 + 1 < cap2) macro_spread_file[pos2++] = '/';
            for (k = 0; base[k] && pos2 + 1 < cap2; k++) macro_spread_file[pos2++] = base[k];
            {
                const char *suf2 = ".am";
                for (k = 0; suf2[k] && pos2 + 1 < cap2; k++) macro_spread_file[pos2++] = suf2[k];
            }
            macro_spread_file[pos2] = '\0';
        }

        /* מריצים את ה-pre-assembler */
        pre_assembler_result = runPreAssembler(input_path, macro_log_path, macro_spread_file, sizeof(macro_spread_file));
        if (pre_assembler_result != 0) {
            printf("no result from pre-assembler for %s.\n", input_path);
            any_fail = 1;
            continue; /* עובר לקובץ הבא */
        }

        /* מריצים את ה-first pass */
        first_pass_res = exe_first_pass(macro_spread_file);
        if (first_pass_res != 0) {
            printf("First pass failed for %s.\n", input_path);
            any_fail = 1;
            continue;
        }

        /* מוחקים את קובץ הלוג של המקרו אם האסמבלר עבד */
        (void)remove(macro_log_path);
    }

    return any_fail ? 1 : 0;
}
