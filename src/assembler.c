#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pre-assembler/pre-assembler.h"
#include "first-run/first-run.h"

static void basename_no_ext(const char *path, char *out, size_t outSize) {
    const char *slash1 = strrchr(path, '/');
    const char *slash2 = strrchr(path, '\\');
    const char *base = slash1 ? slash1 + 1 : (slash2 ? slash2 + 1 : path);
    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    if (len >= outSize) len = outSize - 1;
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

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        char input_path[512];
        char base[256];
        char macro_spread_file[256];
        char macro_log_path[512];
        int pre_assembler_result;
        int first_pass_res;

        /* Use path exactly as provided (no .as handling) */
        {
            size_t cap = sizeof(input_path);
            size_t inlen = strlen(arg);
            if (inlen >= cap) inlen = cap - 1;
            memcpy(input_path, arg, inlen);
            input_path[inlen] = '\0';
        }

        basename_no_ext(input_path, base, sizeof(base));
        /* Build outputs/<base>.macros.txt without snprintf (ANSI C89) */
        {
            const char *prefix = "outputs/";
            const char *suffix = ".macros.txt";
            size_t cap = sizeof(macro_log_path);
            size_t pos = 0;
            size_t j;
            for (j = 0; prefix[j] && pos + 1 < cap; j++) macro_log_path[pos++] = prefix[j];
            for (j = 0; base[j] && pos + 1 < cap; j++) macro_log_path[pos++] = base[j];
            for (j = 0; suffix[j] && pos + 1 < cap; j++) macro_log_path[pos++] = suffix[j];
            macro_log_path[pos] = '\0';
        }

        pre_assembler_result = runPreAssembler(input_path, macro_log_path, macro_spread_file, sizeof(macro_spread_file));
        if (pre_assembler_result != 0) {
            printf("no result from pre-assembler for %s.\n", input_path);
            any_fail = 1;
            continue; /* move on to next file */
        }

        /* macro_spread_file now ends with .am per pre-assembler change */
        first_pass_res = exe_first_pass(macro_spread_file);
        if (first_pass_res != 0) {
            printf("First pass failed for %s.\n", input_path);
            any_fail = 1;
            continue;
        }
    }

    return any_fail ? 1 : 0;
}
