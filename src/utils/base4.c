#include "base4.h"
#include <string.h>


void to_special_base4_str(unsigned short value, char *out) {
    static const char digits[] = {'a', 'b', 'c', 'd'};
    int i;
    for (i = 4; i >= 0; --i) {
        out[i] = digits[value & 0x3]; 
        value >>= 2;
    }
    out[5] = '\0';
}
