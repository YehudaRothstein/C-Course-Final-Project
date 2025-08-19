#include "label_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../error-handler/error-handler.h"

/* מוסיף תווית לטבלת התוויות */
int insert_label(LabelNode **head, const char *label_name, int address, int is_code, int is_data, int is_extern) {
    
    LabelNode *new_node;
    /* בדוק אם התווית כבר קיימת */
    if (find_label(*head, label_name) != NULL) {
        /* אם התווית כבר קיימת, מחזיר שגיאה */
        error_report_ex(ERR_SEV_ERROR, ERR_LABEL_DUPLICATE, NULL, 0, label_name);
        return 0;
    }
    /* מקצה זיכרון לתווית חדשה */
    new_node = (LabelNode *)malloc(sizeof(LabelNode));
    if (!new_node) {
        /* אם לא הצליח להקצות זיכרון, מחזיר שגיאה */
        error_report_ex(ERR_SEV_ERROR, ERR_OUT_OF_MEMORY, NULL, 0, "label node");
        return 0;
    }
    /* מעתיק את שם התווית ומגדיר את שאר השדות */
    strncpy(new_node->name, label_name, MAX_LABEL_LEN - 1);
    new_node->name[MAX_LABEL_LEN - 1] = '\0';
    new_node->address = address;
    new_node->is_code = is_code;
    new_node->is_data = is_data;
    new_node->is_extern = is_extern;
    new_node->is_entry = 0;
    new_node->next = NULL;
    
    /* אם הרשימה ריקה, התווית החדשה היא הראש */
    if (*head == NULL) {
        *head = new_node;
    } else {
        /* אחרת, מוסיף את התווית החדשה בסוף הרשימה */
        LabelNode *curr = *head;
        while (curr->next) curr = curr->next;
        curr->next = new_node;
    }
    return 1;
}

/* מחפש תווית בטבלת התוויות */
LabelNode* find_label(LabelNode *head, const char *label_name) {
    /* עובר על כל התוויות ברשימה */
    while (head) {
        /* אם מצא את התווית, מחזיר את הצומת */
        if (strcmp(head->name, label_name) == 0) return head;
        head = head->next;
    }
    return NULL;
}

int check_duplicate_labels(LabelNode *head) {
    LabelNode *curr;
    for (curr = head; curr; curr = curr->next) {
        LabelNode *other;
        for (other = curr->next; other; other = other->next) {
            if (strcmp(curr->name, other->name) == 0) {
                error_report_ex(ERR_SEV_ERROR, ERR_LABEL_DUPLICATE, NULL, 0, curr->name);
                return 0;
            }
        }
    }
    return 1;
}


void free_label_list(LabelNode *head) {
    LabelNode *curr = head;
    while (curr) {
        LabelNode *next = curr->next;
        free(curr);
        curr = next;
    }
}

void print_symbol_table(LabelNode *head) {
    /* Intentionally no output to keep console identical */
    (void)head;
}