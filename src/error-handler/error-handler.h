
#ifndef ERROR_H
#define ERROR_H

typedef enum {
    ERR_NONE = 0,
    ERR_FILE_NOT_FOUND,
    ERR_INVALID_MACRO_SYNTAX,
    ERR_UNEXPECTED_END_OF_FILE,
    ERR_OUTPUT_FOLDER_MISSING
} ErrorCode;

void handleError(ErrorCode code, const char *details);

#endif
