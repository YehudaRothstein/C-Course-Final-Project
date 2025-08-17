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
    DataDirective *data_directives,    /* Array of data directives collected during the first pass */
    int data_directives_count,        /* Number of data directives in the array */
    int data_base_addr,              /* Base address for the data image */
    data_word **data_image_ptr,      /* Pointer to the data image */
    int *data_count_ptr,             /* Pointer to the data count */
    int *DC_ptr,                     /* Pointer to the current value of DC (data counter) */
    LabelNode **label_table_head_ptr); /* Pointer to the head of the label table */

#endif /* PROCESS_DATA_DIRECTIVES_H */
