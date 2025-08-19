#ifndef OTHER_TABLE_H
#define OTHER_TABLE_H

/* מבנה עבור פריט בטבלת הפריטים האחרים */
typedef struct {
    char name[32];
    int address;
} other_table;

void add_to_other_table(other_table **arr, int *count, const char *name);

#endif
