#ifndef DATA_CONV_H
#define DATA_CONV_H

/* מבנה נתונים לייצוג ערך בינארי */
/* הוא מכיל את הערך הבינארי ואת המידע והאם הוא תורגם או לא */
typedef struct {
    int value;
    int translated;
} data_conv_t;

#endif
