#include <stdlib.h>
#include <string.h>
#include "other_table.h"

void add_to_other_table(other_table **arr, int *count, const char *name) {
    int new_count;
    other_table *tmp;
    if (!arr || !count) return;

    new_count = *count + 1;
    tmp = (other_table *)realloc(*arr, sizeof(other_table) * new_count);
    if (!tmp) return; /* On OOM just skip; error handling elsewhere */

    *arr = tmp;
    memset(&((*arr)[*count]), 0, sizeof(other_table));
    if (name) {
        strncpy((*arr)[*count].name, name, sizeof((*arr)[*count].name) - 1);
        (*arr)[*count].name[sizeof((*arr)[*count].name) - 1] = '\0';
    }
    (*arr)[*count].address = 0;
    *count = new_count;
}
