#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "pre-assembler/pre-assembler.h"
#include "first-run/first-run.h"
#include "utils/utils.h"

/* Track current macro artifacts for cleanup on error/interruption */
static char g_macro_log_path[512];
static char g_macro_spread_file[512];
static int g_has_macro_paths = 0;

/* שימוש מאוחד בפונקציית עזר: get_basefile(utils) */

static void cleanup_macro_artifacts(void) {
    if (g_has_macro_paths) {
        (void)remove(g_macro_log_path);
        (void)remove(g_macro_spread_file);
        g_has_macro_paths = 0;
    }
}

/* On normal success we keep the .am file, remove only the log */
static void cleanup_macro_logs_only(void) {
    if (g_has_macro_paths) {
        (void)remove(g_macro_log_path);
        /* keep g_macro_spread_file (.am) */
    }
}

static void on_interrupt(int sig) {
    (void)sig;
    cleanup_macro_artifacts();
    /* exit to trigger any other atexit handlers if present */
    exit(1);
}

int main(int argc, char *argv[]) {
    int i;
    int any_fail = 0;

    if (argc < 2) {
        printf("Usage: %s <file1> [file2 ...]\n", argv[0]);
        return 1;
    }

    /* On normal exit, remove only macro logs; interrupts remove both */
    atexit(cleanup_macro_logs_only);
    signal(SIGINT, on_interrupt);
    signal(SIGTERM, on_interrupt);
    signal(SIGABRT, on_interrupt);

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

        g_has_macro_paths = 0; /* reset before each file */

        /* השתמש בנתיב כפי שניתן (בלי טיפול ב-.as) */
        {
            size_t cap = sizeof(input_path);
            size_t inlen = strlen(arg);
            /* מעתיקים את הנתיב  */
            if (inlen >= cap) inlen = cap - 1;
            memcpy(input_path, arg, inlen);
            input_path[inlen] = '\0';
        }

    get_basefile(input_path, base, sizeof(base));

    /* הכנה אופציונלית של תיקיית פלט (עשוי להחזיר ".") */
    (void)ensure_output_dir(base, out_dir, sizeof(out_dir));

        /* בונים את שם קובץ לוג המקרו בצורה פשוטה: <base>.macros.txt בספרייה הנוכחית */
        {
            size_t cap = sizeof(macro_log_path);
            macro_log_path[0] = '\0';
            strncat(macro_log_path, base, cap - 1);
            strncat(macro_log_path, ".macros.txt", cap - 1 - strlen(macro_log_path));
        }

        /* קובץ המאקרו הפרוש: <base>.am בספרייה הנוכחית */
        {
            size_t cap2 = sizeof(macro_spread_file);
            macro_spread_file[0] = '\0';
            strncat(macro_spread_file, base, cap2 - 1);
            strncat(macro_spread_file, ".am", cap2 - 1 - strlen(macro_spread_file));
        }

        /* Update globals for cleanup on interruption */
        strncpy(g_macro_log_path, macro_log_path, sizeof(g_macro_log_path) - 1);
        g_macro_log_path[sizeof(g_macro_log_path) - 1] = '\0';
        strncpy(g_macro_spread_file, macro_spread_file, sizeof(g_macro_spread_file) - 1);
        g_macro_spread_file[sizeof(g_macro_spread_file) - 1] = '\0';
        g_has_macro_paths = 1;

        /* מריצים את ה-pre-assembler */
        pre_assembler_result = runPreAssembler(input_path, macro_log_path, macro_spread_file, sizeof(macro_spread_file));
    if (pre_assembler_result != 0) {
            printf("no result from pre-assembler for %s.\n", input_path);
            cleanup_macro_artifacts();
            any_fail = 1;
            continue; /* עובר לקובץ הבא */
        }

        /* macro_spread_file עכשיו מסתיים ב-.am ונמצא בתוך <base>-outputs */
        first_pass_res = exe_first_pass(macro_spread_file);
    if (first_pass_res != 0) {
            printf("First pass failed for %s.\n", input_path);
            cleanup_macro_artifacts();
            any_fail = 1;
            continue;
        }

    /* Success path: keep .am; remove only macro logs */
    cleanup_macro_logs_only();
    }

    return any_fail ? 1 : 0;
}
