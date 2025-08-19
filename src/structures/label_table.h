#ifndef LABEL_TABLE_H
#define LABEL_TABLE_H

#define MAX_LABEL_LEN 31

/* צומת בתור תווית */
typedef struct LabelNode {
    char name[MAX_LABEL_LEN];
    int address;
    int is_code;
    int is_data;
    int is_extern;
    int is_entry;
    struct LabelNode *next;
} LabelNode;

/* מוסיף תווית לטבלת התוויות */
int insert_label(LabelNode **head, const char *label_name, int address, int is_code, int is_data, int is_extern);


/* מחפש תווית בטבלת התוויות */
LabelNode* find_label(LabelNode *head, const char *label_name);


void update_data_labels_address(LabelNode *head);


int check_duplicate_labels(LabelNode *head);


void free_label_list(LabelNode *head);


void print_symbol_table(LabelNode *head);

#endif