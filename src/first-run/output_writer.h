#ifndef OUTPUT_WRITER_H
#define OUTPUT_WRITER_H


#include "code_conversion.h"
#include "data_conv.h"
#include "data_word.h"

void write_code_file(const char *out_filename, code_conv *code, int code_count, data_word *data_image, int data_count);

#endif
