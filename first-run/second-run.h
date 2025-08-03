#ifndef SECOND_RUN_H
#define SECOND_RUN_H

#include "label_table.h"
#include "other_table.h"
#include "code_conversion.h"

void exe_second_pass(
    code_conv *code, int code_count,
    LabelNode *label_table,
    other_table *entries, int entries_count,
    other_table *externs, int externs_count,
    const char *base_filename
);

#endif // SECOND_RUN_H
