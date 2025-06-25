#include <ctype.h>
#include "utils.h"

char* ltrim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    return str;
}

int startsWithIgnoreCase(const char* str, const char* prefix) {
    while (*prefix && *str && tolower((unsigned char)*str) == tolower((unsigned char)*prefix)) {
        str++;
        prefix++;
    }
    return *prefix == '\0';
}
