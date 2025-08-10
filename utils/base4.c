#include "base4.h"
#include <string.h>

// Converts a 10-bit unsigned value to a 5-char string in the special base-4 encoding (a-d, left-padded with 'a').
void to_special_base4_str(unsigned short value, char *out) {
    static const char digits[] = {'a', 'b', 'c', 'd'};
    int i;
    for (i = 4; i >= 0; --i) {
        out[i] = digits[value & 0x3]; // Take 2 bits
        value >>= 2;
    }
    out[5] = '\0';
}
