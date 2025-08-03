#include "label_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Insert a new label node to the list. Returns 1 on success, 0 on duplicate.
int insert_label(LabelNode **head, const char *label_name, int address, int is_code, int is_data, int is_extern) {
    if (find_label(*head, label_name) != NULL) {
        fprintf(stderr, "Error: Duplicate label '%s'\n", label_name);
        return 0;
    }
    LabelNode *new_node = (LabelNode *)malloc(sizeof(LabelNode));
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation failed for label node\n");
        return 0;
    }
    strncpy(new_node->name, label_name, MAX_LABEL_LEN - 1);
    new_node->name[MAX_LABEL_LEN - 1] = '\0';
    new_node->address = address;
    new_node->is_code = is_code;
    new_node->is_data = is_data;
    new_node->is_extern = is_extern;
    new_node->is_entry = 0;
    new_node->next = NULL;
    // Insert at end
    if (*head == NULL) {
        *head = new_node;
    } else {
        LabelNode *curr = *head;
        while (curr->next) curr = curr->next;
        curr->next = new_node;
    }
    return 1;
}

// Find a label node by name
LabelNode* find_label(LabelNode *head, const char *label_name) {
    while (head) {
        if (strcmp(head->name, label_name) == 0) return head;
        head = head->next;
    }
    return NULL;
}

// Update data label addresses after first pass (add final IC to all data labels)
void update_data_labels_address(LabelNode *head) {
    // This function should be called after final IC is known and data labels need to be updated
    // The actual value to add should be passed in as a parameter if needed
    // For now, this is a placeholder; update as needed in integration
    // Example: for (LabelNode *curr = head; curr; curr = curr->next) { if (curr->is_data) curr->address += final_IC; }
}

// Check for duplicate labels in the list. Returns 1 if no duplicates, 0 if found.
int check_duplicate_labels(LabelNode *head) {
    for (LabelNode *curr = head; curr; curr = curr->next) {
        for (LabelNode *other = curr->next; other; other = other->next) {
            if (strcmp(curr->name, other->name) == 0) {
                fprintf(stderr, "Error: Duplicate label '%s' found in label table\n", curr->name);
                return 0;
            }
        }
    }
    return 1;
}

// Free the label list
void free_label_list(LabelNode *head) {
    LabelNode *curr = head;
    while (curr) {
        LabelNode *next = curr->next;
        free(curr);
        curr = next;
    }
}