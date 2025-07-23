#include "label_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Label *label_table = NULL;
int label_count = 0;

int label_exists(const char *label_name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(label_table[i].name, label_name) == 0)
            return 1;
    }
    return 0;
}

int insert_label_table(const char *label_name, int address, int is_code, int is_data, int is_extern, int is_entry) {
    if (label_exists(label_name)) {
        fprintf(stderr, "Error: Duplicate label '%s'\n", label_name);
        return 0;
    }
    Label *new_table = realloc(label_table, (label_count + 1) * sizeof(Label));
    if (!new_table) {
        fprintf(stderr, "Error: Memory allocation failed for label table\n");
        return 0;
    }
    label_table = new_table;
    strncpy(label_table[label_count].name, label_name, MAX_LABEL_LEN - 1);
    label_table[label_count].name[MAX_LABEL_LEN - 1] = '\0';
    label_table[label_count].address = address;
    label_table[label_count].is_code = is_code;
    label_table[label_count].is_data = is_data;
    label_table[label_count].is_extern = is_extern;
    label_table[label_count].is_entry = is_entry;
    label_count++;
    return 1;
}

void update_data_labels_address(int final_IC) {
    for (int i = 0; i < label_count; i++) {
        if (label_table[i].is_data) {
            label_table[i].address += final_IC;
        }
    }
}

void free_label_table() {
    free(label_table);
    label_table = NULL;
    label_count = 0;
}