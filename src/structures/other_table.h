#ifndef OTHER_TABLE_H
#define OTHER_TABLE_H

typedef struct {
    char name[32];
    int address;
} other_table;

/* Append a name to the dynamic other_table array, growing as needed */
void add_to_other_table(other_table **arr, int *count, const char *name);

#endif
