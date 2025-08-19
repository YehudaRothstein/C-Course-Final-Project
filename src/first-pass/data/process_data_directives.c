#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "process_data_directives.h"
#include "memory_map_data.h"
#include "memory_map.h"
#include "../../error-handler/error-handler.h"

/* פונקציה לעיבוד מספרים שלמים מתוך מחרוזת */
static int parse_int_and_advance(const char **pcursor, int *out_value) {
    const char *p;
    int sign;
    long acc;

    if (!pcursor || !*pcursor || !out_value) return 0;
    p = *pcursor;

    /* דילוג על רווחים */
    while (*p && isspace((unsigned char)*p)) p++;

    /* סימן אופציונלי */
    sign = 1;
    if (*p == '+') { p++; }
    else if (*p == '-') { sign = -1; p++; }

    /* חייב להיות לפחות ספרה אחת */
    if (!isdigit((unsigned char)*p)) return 0;

    acc = 0;
    while (isdigit((unsigned char)*p)) {
        acc = acc * 10 + (*p - '0');
        p++;
    }

    acc *= sign;
    *out_value = (int)acc; /* עבור קלטי המטלה זה מספיק */
    *pcursor = p;          /* קידום המצביע למיקום הבא */
    return 1;
}

/* מציאת מידות מטריצה מהטקסט */
static int parse_matrix_dimensions(const char *text, int *rows, int *cols) {
    const char *cursor = text;

    /* מוצא את ה '[' הראשון */
    while (*cursor && *cursor != '[') {
        cursor++;
    }
    /* אם לא נמצא '[', מחזירים 0 */
    if (*cursor != '[') {
        return 0;
    }
    cursor++;

    if (!parse_int_and_advance(&cursor, rows)) {
        return 0;
    }

    /* מוצא את ה '[' השני */
    while (*cursor && *cursor != '[') {
        cursor++;
    }
    if (*cursor != '[') {
        return 0;
    }
    cursor++;

    if (!parse_int_and_advance(&cursor, cols)) {
        return 0;
    }

    /* רק אם המידות חיוביות */
    return (*rows > 0 && *cols > 0);
}

/* עיבוד ערכים מספריים */
static int handle_data_directive_data(
    DataParts *directive,
    int data_base_addr,
    data_word *data_image_array,
    int *data_counter_ptr,
    int *data_word_count_ptr,
    LabelNode **label_head_ptr)
{
    int start_index = *data_counter_ptr;
    const char *ptr;
    int write_index;

    /* אם יש תווית, מוסיפים אותה למפה */
    if (directive->label[0]) {
        insert_label(label_head_ptr, directive->label, data_base_addr + *data_counter_ptr, 0, 1, 0);
        start_index = *data_counter_ptr;
    }

    ptr = directive->operands;
    write_index = start_index;

    /* עיבוד ערכים מספריים */
    while (*ptr) {
        /* דילוג על רווחים */
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
        if (!*ptr) break;

        /* אם יש פסיק, מדלגים עליו */
        if (*ptr == ',') { ptr++; continue; }
        {
            int value;
            if (!parse_int_and_advance(&ptr, &value)) {
                break; /* אין מספר חוקי – יוצאים מהלולאה */
            }
            if ((data_base_addr + *data_counter_ptr) >= MEMORY_SIZE) {
                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".data exceedes memory size");
                return 0;
            }
            /* שמירת הערך במערך */
            data_image_array[write_index].value = (unsigned short)value;
            /* קביעת מספר השורה של המקור */
            data_image_array[write_index].src_line = directive->src_line;

            /* קידום המצביע למיקום הבא */
            write_index++;    
            (*data_counter_ptr)++;
            (*data_word_count_ptr)++;
        }
    }
    return 1;
}

/* עיבוד מחרוזות */
static int handle_data_directive_string(
    DataParts *directive,
    int data_base_addr,
    data_word *data_image_array,
    int *data_counter_ptr,
    int *data_word_count_ptr,
    LabelNode **label_head_ptr)
{
    int start_index = *data_counter_ptr;
    const char *p;
    int write_index;

    /* אם יש תווית, מוסיפים אותה למפה */
    if (directive->label[0]) {
        insert_label(label_head_ptr, directive->label, data_base_addr + *data_counter_ptr, 0, 1, 0);
        start_index = *data_counter_ptr;
    }

    p = directive->operands;
    write_index = start_index;

    /* חיפוש לציטוט הראשון */
    while (*p && *p != '"') p++;
    if (*p != '"') {
        error_report_ex(ERR_SEV_ERROR, ERR_STRING_NOT_QUOTED, NULL, directive->src_line, NULL);
        return 0;
    }
    p++;

    /* חיפוש אחר הציטוט השני */
    while (*p && *p != '"') {
        /* בדיקת תו חוקי */
        unsigned char ch = (unsigned char)*p;
        if (ch < 32 || ch > 126) {
            error_report_ex(ERR_SEV_ERROR, ERR_STRING_BAD_CHAR, NULL, directive->src_line, NULL);
            return 0;
        }
        /* בדיקת גודל מפת הנתונים */
        if ((data_base_addr + *data_counter_ptr) >= MEMORY_SIZE) {
            error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".string exceeds memory size");
            return 0;
        }
        /* שמירת הערך במערך */
        data_image_array[write_index].value = (unsigned short)ch;
        data_image_array[write_index].src_line = directive->src_line;

        /* קידום המצביע למיקום הבא */
        write_index++;
        (*data_counter_ptr)++;
        (*data_word_count_ptr)++;
        p++;
    }

    /* קביעת סוף המחרוזת */
    if ((data_base_addr + *data_counter_ptr) >= MEMORY_SIZE) {
        error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".string exceeds memory size");
        return 0;
    }
    data_image_array[write_index].value = 0;
    data_image_array[write_index].src_line = directive->src_line;
    write_index++;
    (*data_counter_ptr)++;
    (*data_word_count_ptr)++;
    return 1;
}

/* עיבוד מטריצות (גרסה פשוטה) */
static int handle_data_directive_mat(
    DataParts *directive,
    int data_base_addr,
    data_word *data_image_array,
    int *data_counter_ptr,
    int *data_word_count_ptr,
    LabelNode **label_head_ptr)
{
    int start_index = *data_counter_ptr;
    int rows = 0, cols = 0, total = 0;
    const char *ptr;
    int close_brackets_seen;

    /* אם יש תווית */
    if (directive->label[0]) {
        insert_label(label_head_ptr, directive->label, data_base_addr + *data_counter_ptr, 0, 1, 0);
        start_index = *data_counter_ptr;
    }

    /* בדיקת ממדים */
    if (!parse_matrix_dimensions(directive->operands, &rows, &cols)) {
        error_report_ex(ERR_SEV_ERROR, ERR_MAT_SIZE_INVALID, NULL, directive->src_line, NULL);
        return 0;
    }
    total = rows * cols;

    /* מעבר אחרי שני הסוגריים ] ] */
    ptr = directive->operands;
    close_brackets_seen = 0;
    while (*ptr) {
        if (*ptr == ']') {
            close_brackets_seen++;
            if (close_brackets_seen == 2) { ptr++; break; }
        }
        ptr++;
    }

    /* דילוג על רווחים ופסיק */
    while (*ptr && isspace((unsigned char)*ptr)) ptr++;
    if (*ptr == ',') { ptr++; }
    while (*ptr && isspace((unsigned char)*ptr)) ptr++;

    /* קריאת ערכים עד שמילאנו הכול */
    {
        int filled = 0;
        while (*ptr && filled < total) {
            int num;

            /* דילוג על רווחים ופסיקים */
            while (*ptr && isspace((unsigned char)*ptr)) ptr++;
            if (!*ptr) break;
            if (*ptr == ',') { ptr++; continue; }

            /* קריאת מספר פשוטה */
            if (!parse_int_and_advance(&ptr, &num)) {
                break; /* אין יותר מספרים */
            }

            /* בדיקת זיכרון */
            if ((data_base_addr + *data_counter_ptr) >= MEMORY_SIZE) {
                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".mat exceeds memory size");
                return 0;
            }

            /* כתיבת הערך */
            data_image_array[start_index + filled].value = (unsigned short)num;
            data_image_array[start_index + filled].src_line = directive->src_line;
            filled++;
            (*data_counter_ptr)++;
            (*data_word_count_ptr)++;

            /* דילוג נוסף על רווחים/פסיק */
            while (*ptr && isspace((unsigned char)*ptr)) ptr++;
            if (*ptr == ',') ptr++;
        }

        /* מילוי יתרת התאים באפס */
        while (filled < total) {
            if ((data_base_addr + *data_counter_ptr) >= MEMORY_SIZE) {
                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, NULL, directive->src_line, ".mat exceeds memory size");
                return 0;
            }
            data_image_array[start_index + filled].value = 0;
            data_image_array[start_index + filled].src_line = directive->src_line;
            filled++;
            (*data_counter_ptr)++;
            (*data_word_count_ptr)++;
        }

        /* אזהרה על עודפים: אם נשאר עוד מספר אחרי שמילאנו הכול */
        {
            const char *check = ptr;
            while (*check && isspace((unsigned char)*check)) check++;
            if (*check == ',') { check++; while (*check && isspace((unsigned char)*check)) check++; }
            {
                int extra;
                const char *tmp = check;
                if (parse_int_and_advance(&tmp, &extra)) {
                    error_report_ex(ERR_SEV_WARNING, ERR_MAT_INIT_COUNT_MISMATCH, NULL, directive->src_line, "extra initializers ignored");
                }
            }
        }
    }

    return 1;
}

/* פונקציה לעיבוד הנחיות נתונים */
void process_data_directives(
    DataParts *data_directives,
    int data_directives_count,
    int data_base_addr,
    data_word **data_image_ptr,
    int *data_count_ptr,
    int *DC_ptr,
    LabelNode **label_table_head_ptr)
{
    int data_counter = *DC_ptr;                 /* DC – מונה נתונים */
    int data_word_count = *data_count_ptr;      /* מספר המילים שהוקצו לנתונים */
    data_word *data_image_array = *data_image_ptr; /* מערך תמונת הנתונים */
    LabelNode *label_head = *label_table_head_ptr;  /* טבלת תוויות */
    int directive_index;

    for (directive_index = 0; directive_index < data_directives_count; directive_index++) {
        DataParts *directive = &data_directives[directive_index];

        if (strcmp(directive->type, ".data") == 0) {
            if (!handle_data_directive_data(directive, data_base_addr, data_image_array,
                                           &data_counter, &data_word_count, &label_head)) {
                break; 
            }
        } else if (strcmp(directive->type, ".string") == 0) {
            if (!handle_data_directive_string(directive, data_base_addr, data_image_array,
                                             &data_counter, &data_word_count, &label_head)) {
                break;
            }
        } else if (strcmp(directive->type, ".mat") == 0) {
            if (!handle_data_directive_mat(directive, data_base_addr, data_image_array,
                                          &data_counter, &data_word_count, &label_head)) {
                break;
            }
        }
    }

    *data_image_ptr = data_image_array;
    *data_count_ptr = data_word_count;
    *DC_ptr = data_counter;
    *label_table_head_ptr = label_head;
}
