#ifndef MEMORY_MAP_PRINT_H
#define MEMORY_MAP_PRINT_H



#include "code_conversion.h"
#include "data_conv.h"
#include "data_word.h"
#include "label_table.h"

void print_memory_map(int code_count, code_conv *code, int data_count, data_word *data_image, LabelNode *label_table_head);

#endif
