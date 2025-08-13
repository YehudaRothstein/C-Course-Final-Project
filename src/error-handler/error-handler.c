#include <stdio.h>
#include "error-handler.h"

void handleError(ErrorCode code, const char *details) {
    switch (code) {
        case ERR_NONE:
            break;

        case ERR_FILE_NOT_FOUND:
            printf("Error: Input file not found: %s\n", details);
            printf("Please check the input file path and try again.\n");
            break;

        case ERR_INVALID_MACRO_SYNTAX:
            printf("Error: Invalid macro syntax near: %s\n", details);
            printf("Make sure your macro definitions start with 'mcro <name>' and end with 'mcroend'.\n");
            break;

        case ERR_UNEXPECTED_END_OF_FILE:
            printf("Error: Unexpected end of file while processing macros.\n");
            printf("Make sure every 'mcro' has a matching 'mcroend'.\n");
            break;

        case ERR_OUTPUT_FOLDER_MISSING:
            printf("Error: Output folder is missing or not writable.\n");
            printf("Please create the output directory (e.g., 'outputs') and check permissions.\n");
            break;

        default:
            printf("Unknown error occurred.\n");
            break;
    }
}
