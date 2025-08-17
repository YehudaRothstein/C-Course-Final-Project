#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "error-handler.h"

/* המשתנה הזה סופר את מספר השגיאות */
static int g_error_count = 0;

/* המשתנה הזה סופר את מספר האזהרות */
static int g_warning_count = 0;

int error_get_error_count(void) { return g_error_count; }
int error_get_warning_count(void) { return g_warning_count; }
void error_reset_counts(void) { g_error_count = 0; g_warning_count = 0;  }

static const char* code_to_string(ErrorCode code) {
    switch (code) {
        case ERR_NONE: return "GOOD:)";
        case ERR_CLI_NO_INPUT: return "No input files provided";
        case ERR_CLI_BAD_EXTENSION: return "Input file must have .as extension";
        case ERR_IO_INPUT_OPEN_FAIL: return "Failed to open input file";
        case ERR_IO_OUTPUT_OPEN_FAIL: return "Failed to open output file";
        case ERR_IO_PATH_INVALID: return "Invalid path";
        case ERR_LINE_TOO_LONG: return "Line is longer than 80 characters";
        case ERR_INVALID_CHAR: return "Invalid character in source";
        case ERR_MACRO_SYNTAX: return "Invalid macro syntax";
        case ERR_MACRO_UNTERMINATED: return "Missing macro end";
        case ERR_MACRO_DUPLICATE_NAME: return "Duplicate macro name";
        case ERR_MACRO_NAME_RESERVED: return "Macro name is reserved";
        case ERR_MACRO_USE_UNDEFINED: return "Use of undefined macro";
        case ERR_TRAILING_TEXT: return "Trailing text after valid statement";
        case ERR_COMMA_MISSING: return "Missing comma between operands";
        case ERR_COMMA_EXTRA: return "Extra or misplaced comma";
        case ERR_BRACKETS_MISMATCH: return "Mismatched or missing brackets";
        case ERR_QUOTE_MISSING: return "Missing closing quote";
        case ERR_LABEL_EMPTY: return "Empty or NULL label";
        case ERR_LABEL_TOO_LONG: return "Label too long (max 30)";
        case ERR_LABEL_NOT_START_ALPHA: return "Label must start with a letter";
        case ERR_LABEL_NON_ALNUM: return "Label contains non-alphanumeric characters";
        case ERR_LABEL_RESERVED_WORD: return "Label is a reserved word (opcode/instruction)";
        case ERR_LABEL_IS_REGISTER: return "Label collides with register name (r0-r7)";
        case ERR_LABEL_DUPLICATE: return "Duplicate label";
        case ERR_LABEL_REDEFINED: return "Label redefined";
        case ERR_LABEL_UNDEFINED: return "Undefined label";
        case ERR_DATA_LIST_EMPTY: return "Empty list in .data";
        case ERR_DATA_NUMBER_INVALID: return "Invalid number in .data";
        case ERR_DATA_NUMBER_RANGE: return "Number out of range for 10-bit word";
        case ERR_STRING_NOT_QUOTED: return ".string argument must be quoted";
        case ERR_STRING_BAD_CHAR: return "Invalid character in string";
        case ERR_MAT_SIZE_INVALID: return "Invalid .mat size";
        case ERR_MAT_INIT_COUNT_MISMATCH: return "Mismatch between .mat size and init values";
        case ERR_MAT_INIT_RANGE: return "Matrix init value out of range";
        case ERR_ENTRY_UNDEFINED: return ".entry refers to undefined symbol";
        case ERR_EXTERN_LOCAL_CONFLICT: return ".extern conflicts with a local definition";
        case ERR_EXTERN_DUPLICATE: return "Duplicate .extern declaration";
        case ERR_OPCODE_UNKNOWN: return "Unknown opcode";
        case ERR_OPERAND_COUNT_MISMATCH: return "Wrong number of operands";
        case ERR_ADDR_MODE_INVALID_SRC: return "Illegal addressing mode (source)";
        case ERR_ADDR_MODE_INVALID_DST: return "Illegal addressing mode (destination)";
        case ERR_MATRIX_SYNTAX: return "Illegal matrix operand syntax";
        case ERR_MATRIX_INDEX_NOT_REGISTER: return "Matrix parameters must be registers r0-r7";
        case ERR_MATRIX_LABEL_NOT_DEFINED_AS_MAT: return "Matrix label not defined by .mat";
        case ERR_MATRIX_MODE_NOT_ALLOWED: return "Matrix addressing mode not allowed for opcode";
        case ERR_IMMEDIATE_RANGE: return "Immediate value out of 8-bit range";
        case ERR_DATA_RANGE: return "Data value out of 10-bit range";
        case ERR_NUMBER_FORMAT: return "Invalid number format";
        case ERR_PASS1_FAILED: return "First pass failed";
        case ERR_PASS2_FAILED: return "Second pass failed";
        case ERR_SYMBOL_TABLE_INCONSISTENT: return "Symbol table inconsistency";
        case ERR_MEMORY_OVERFLOW: return "Memory image exceeds capacity";
        case ERR_ENCODING_LENGTH_MISMATCH: return "Instruction encoding length mismatch";
        case ERR_OUT_OF_MEMORY: return "Out of memory";
        case ERR_OUTPUT_OB_WRITE_FAIL: return "Failed writing .ob file";
        case ERR_OUTPUT_BIN_WRITE_FAIL: return "Failed writing .bin file";
        case ERR_OUTPUT_ENT_WRITE_FAIL: return "Failed writing .ent file";
        case ERR_OUTPUT_EXT_WRITE_FAIL: return "Failed writing .ext file";
        case ERR_OUTPUT_EMPTY_ENT_EXT_FORBIDDEN: return "Creating empty .ent/.ext is forbidden";
        default: return "Somesort of error :(";
    }
}
/* הפונקצייה הזו ממפה את רמות החומרהשל השגיאה למחרוזות */
static const char* sev_to_string(ErrorSeverity sev) {
    switch (sev) {
        case ERR_SEV_INFO: return "INFO";
        case ERR_SEV_WARNING: return "WARNING";
        case ERR_SEV_ERROR: return "ERROR";
        default: return "?";
    }
}

/* הפונקציה הזו מדווחת על שגיאה עם פרטים נוספים */
/* היא מקבלת את רמת החומרה, קוד השגיאה, שם הקובץ, מספר השורה ופורמט נוסף */
void error_report_ex(ErrorSeverity sev, ErrorCode code, const char *filename, int line, const char *details) {
    if (sev == ERR_SEV_ERROR) {
        g_error_count++;
    } else if (sev == ERR_SEV_WARNING ) {
        g_warning_count++;
    }

    fprintf(stdout, "[%s] %s", sev_to_string(sev), code_to_string(code));

    /* אם יש שם קובץ ומספר שורה, אנחנו מדפיסים אותם */
    if (filename) {
        if (line > 0) {
            fprintf(stdout, "  (file: %s, line: %d)", filename, line);
        } else {
            fprintf(stdout, " (file:  %s)", filename);
        }
    }

    /* אם יש פרטים נוספים, אנחנו מדפיסים אותם */
    if (details && *details) {
        fprintf(stdout, ": %s", details);
    }

    fprintf(stdout, "\n");
}

/* הפונקציה הזו מדווחת על שגיאה עם פרטים נוספים */
/* היא מקבלת את קוד השגיאה, שם הקובץ, מספר השורה ופורמט נוסף */
void error_report(ErrorCode code, const char *filename, int line, const char *details) {
    /* אם יש שם קובץ ומספר שורה, אנחנו מדפיסים אותם */
    if (details && *details) {
        error_report_ex(ERR_SEV_ERROR, code, filename, line, details);
    } else {
        error_report_ex(ERR_SEV_ERROR, code, filename, line, NULL);
    }
}

/* הפונקציה הזו מטפלת בשיגאות שדווחו */
void handleError(ErrorCode code, const char *details) {
    /* אם קוד השגיאה הוא ERR_NONE, אנחנו לא עושים כלום */
    switch (code) {
        case ERR_NONE:
            return;
        case ERR_IO_INPUT_OPEN_FAIL:
            error_report(ERR_IO_INPUT_OPEN_FAIL, NULL, 0, details ? details : "");
            break;
        case ERR_MACRO_SYNTAX:
            error_report(ERR_MACRO_SYNTAX, NULL, 0, details ? details : "");
            break;
        case ERR_MACRO_UNTERMINATED:
            error_report(ERR_MACRO_UNTERMINATED, NULL, 0, details ? details : "");
            break;
        case ERR_IO_OUTPUT_OPEN_FAIL:
            error_report(ERR_IO_OUTPUT_OPEN_FAIL, NULL, 0, details ? details : "");
            break;
        default:
            error_report_ex(ERR_SEV_ERROR, code, NULL, 0, details ? details : "");
            break;
    }
}
