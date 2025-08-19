#ifndef DATA_WORD_H
#define DATA_WORD_H

/* מבנה נתונים לשמירת מידע על מילה */
typedef struct {
    unsigned short value;
    int src_line;
} data_word;

#endif
