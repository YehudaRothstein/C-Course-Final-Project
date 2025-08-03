#ifndef LABEL_TABLE_H
#define LABEL_TABLE_H

#define MAX_LABEL_LEN 31

typedef struct LabelNode {
    char name[MAX_LABEL_LEN];
    int address;
    int is_code;
    int is_data;
    int is_extern;
    int is_entry;
    struct LabelNode *next;
} LabelNode;

// Insert a new label node to the list. Returns 1 on success, 0 on duplicate.
int insert_label(LabelNode **head, const char *label_name, int address, int is_code, int is_data, int is_extern);

// Find a label node by name
LabelNode* find_label(LabelNode *head, const char *label_name);

// Update data label addresses after first pass (add final IC to all data labels)
void update_data_labels_address(LabelNode *head);

// Check for duplicate labels in the list. Returns 1 if no duplicates, 0 if found.
int check_duplicate_labels(LabelNode *head);

// Free the label list
void free_label_list(LabelNode *head);

#endif