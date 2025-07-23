#ifndef LABEL_TABLE_H
#define LABEL_TABLE_H

#define MAX_LABEL_LEN 31

typedef struct {
    char name[MAX_LABEL_LEN];
    int address;
    int is_code;
    int is_data;
    int is_extern;
    int is_entry;
} Label;

extern Label *label_table;
extern int label_count;

// מוסיף תווית חדשה לטבלה
int insert_label_table(const char *label_name, int address, int is_code, int is_data, int is_extern, int is_entry);

// בודק אם תווית קיימת כבר
int label_exists(const char *label_name);

// עדכון כתובות של תוויות נתונים בסוף המעבר הראשון
void update_data_labels_address(Label *label_table, int label_table_line, int final_IC);

// ניקוי הטבלה בסוף
void free_label_table();

#endif