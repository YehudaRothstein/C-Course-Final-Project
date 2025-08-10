#ifndef BASE4_H
#define BASE4_H

// Converts a 10-bit unsigned value to a 5-char string in the special base-4 encoding (a-d, left-padded with 'a').
// Output buffer must be at least 6 bytes (5 chars + null terminator).
void to_special_base4_str(unsigned short value, char *out);

#endif
