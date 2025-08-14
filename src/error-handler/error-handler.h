#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

/* Severity levels */
typedef enum {
    ERR_SEV_INFO = 0,
    ERR_SEV_WARNING = 1,
    ERR_SEV_ERROR = 2
} ErrorSeverity;

/* Comprehensive error codes catalog */
typedef enum {
    ERR_NONE = 0,

    /* CLI / Arguments */
    ERR_CLI_NO_INPUT,
    ERR_CLI_BAD_EXTENSION,

    /* I/O */
    ERR_IO_INPUT_OPEN_FAIL,
    ERR_IO_OUTPUT_OPEN_FAIL,
    ERR_IO_PATH_INVALID,

    /* Pre-assembler / General line issues */
    ERR_LINE_TOO_LONG,
    ERR_INVALID_CHAR,

    /* Macro processing */
    ERR_MACRO_SYNTAX,
    ERR_MACRO_UNTERMINATED,
    ERR_MACRO_DUPLICATE_NAME,
    ERR_MACRO_NAME_RESERVED,
    ERR_MACRO_USE_UNDEFINED,

    /* Lexing/Parsing */
    ERR_TRAILING_TEXT,
    ERR_COMMA_MISSING,
    ERR_COMMA_EXTRA,
    ERR_BRACKETS_MISMATCH,
    ERR_QUOTE_MISSING,

    /* Labels / Symbols */
    ERR_LABEL_EMPTY,
    ERR_LABEL_TOO_LONG,
    ERR_LABEL_NOT_START_ALPHA,
    ERR_LABEL_NON_ALNUM,
    ERR_LABEL_RESERVED_WORD,
    ERR_LABEL_IS_REGISTER,
    ERR_LABEL_DUPLICATE,
    ERR_LABEL_REDEFINED,
    ERR_LABEL_UNDEFINED,

    /* Directives: .data */
    ERR_DATA_LIST_EMPTY,
    ERR_DATA_NUMBER_INVALID,
    ERR_DATA_NUMBER_RANGE,

    /* Directives: .string */
    ERR_STRING_NOT_QUOTED,
    ERR_STRING_BAD_CHAR,

    /* Directives: .mat */
    ERR_MAT_SIZE_INVALID,
    ERR_MAT_INIT_COUNT_MISMATCH,
    ERR_MAT_INIT_RANGE,

    /* .entry / .extern */
    ERR_ENTRY_UNDEFINED,
    ERR_EXTERN_LOCAL_CONFLICT,
    ERR_EXTERN_DUPLICATE,

    /* Instructions / opcodes */
    ERR_OPCODE_UNKNOWN,
    ERR_OPERAND_COUNT_MISMATCH,
    ERR_ADDR_MODE_INVALID_SRC,
    ERR_ADDR_MODE_INVALID_DST,

    /* Matrix addressing */
    ERR_MATRIX_SYNTAX,
    ERR_MATRIX_INDEX_NOT_REGISTER,
    ERR_MATRIX_LABEL_NOT_DEFINED_AS_MAT,
    ERR_MATRIX_MODE_NOT_ALLOWED,

    /* Numbers / ranges */
    ERR_IMMEDIATE_RANGE,
    ERR_DATA_RANGE,
    ERR_NUMBER_FORMAT,

    /* Passes / symbol table */
    ERR_PASS1_FAILED,
    ERR_PASS2_FAILED,
    ERR_SYMBOL_TABLE_INCONSISTENT,

    /* Memory / encoding */
    ERR_MEMORY_OVERFLOW,
    ERR_ENCODING_LENGTH_MISMATCH,
    ERR_OUT_OF_MEMORY,

    /* Outputs */
    ERR_OUTPUT_OB_WRITE_FAIL,
    ERR_OUTPUT_BIN_WRITE_FAIL,
    ERR_OUTPUT_ENT_WRITE_FAIL,
    ERR_OUTPUT_EXT_WRITE_FAIL,
    ERR_OUTPUT_EMPTY_ENT_EXT_FORBIDDEN
} ErrorCode;

/* Legacy aliases for backward compatibility */
#ifndef ERR_FILE_NOT_FOUND
#define ERR_FILE_NOT_FOUND ERR_IO_INPUT_OPEN_FAIL
#endif
#ifndef ERR_INVALID_MACRO_SYNTAX
#define ERR_INVALID_MACRO_SYNTAX ERR_MACRO_SYNTAX
#endif
#ifndef ERR_UNEXPECTED_END_OF_FILE
#define ERR_UNEXPECTED_END_OF_FILE ERR_MACRO_UNTERMINATED
#endif
#ifndef ERR_OUTPUT_FOLDER_MISSING
#define ERR_OUTPUT_FOLDER_MISSING ERR_IO_OUTPUT_OPEN_FAIL
#endif

/* Reporting API */
void error_report_ex(ErrorSeverity sev, ErrorCode code, const char *filename, int line, const char *fmt, ...);
void error_report(ErrorCode code, const char *filename, int line, const char *details);

/* Counters */
int error_get_error_count(void);
int error_get_warning_count(void);
void error_reset_counts(void);

/* Backward compatible (legacy) API used in some places) */
void handleError(ErrorCode code, const char *details);

#endif
