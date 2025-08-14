#ifndef PROCESS_DATA_DIRECTIVES_H
#define PROCESS_DATA_DIRECTIVES_H

#include "data_directive.h"
#include "data_word.h"
#include "label_table.h"

/*
 * Processes deferred data directives (.data, .string, .mat) collected during the first pass.
 * Builds the data image, updates DC and inserts data labels into the label table.
 */
void process_data_directives(
    DataDirective *data_directives,
    int data_directives_count,
    int data_base_addr,
    data_word **data_image_ptr,
    int *data_count_ptr,
    int *DC_ptr,
    LabelNode **label_table_head_ptr);

#endif /* PROCESS_DATA_DIRECTIVES_H */
