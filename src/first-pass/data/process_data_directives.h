#ifndef PROCESS_DATA_DIRECTIVES_H
#define PROCESS_DATA_DIRECTIVES_H

#include "data_directive.h"
#include "data_word.h"
#include "label_table.h"

/*
 * פונקציה לעיבוד פקודות נתונים
 */
void process_data_directives(
    DataParts *data_directives,    /* מערך של פקודות נתונים שנאספו במהלך המעבר הראשון */
    int data_directives_count,        /* מספר פקודות הנתונים במערך */
    int data_base_addr,              /* כתובת הבסיס עבור תמונת הנתונים */
    data_word **data_image_ptr,      /* מצביע לתמונת הנתונים */
    int *data_count_ptr,             /* מצביע למספר הנתונים */
    int *DC_ptr,                     /* מצביע לערך הנוכחי של DC (סופר הנתונים) */
    LabelNode **label_table_head_ptr); /* מצביע לראש טבלת התוויות */

#endif 
