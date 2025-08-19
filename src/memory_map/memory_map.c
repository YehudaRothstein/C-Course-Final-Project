#include "memory_map.h"
#include <stdio.h>
#include <string.h>

#define WORD_MASK 0x3FF /* 10 bits */

/* אתחול מפה זיכרון */
void memory_map_init(MemoryMap *map) {
    memset(map, 0, sizeof(MemoryMap));
}

/* עדכון כתובת במפה */
void memory_map_set(MemoryMap *map, size_t address, word_t value) {
    if (address < MEMORY_SIZE) {
    map->cells[address].value = (word_t)(value & WORD_MASK);
        map->cells[address].used = 1;
        if (address + 1 > map->size) {
            map->size = address + 1;
        }
    }
}

/* קבלת ערך מכתובת במפה */
word_t memory_map_get(const MemoryMap *map, size_t address) {
    if (address < MEMORY_SIZE && map->cells[address].used) {
        return map->cells[address].value;
    }
    return 0;
}

/* סימון כתובת בשימוש */
void memory_map_mark_used(MemoryMap *map, size_t address) {
    if (address < MEMORY_SIZE) {
        map->cells[address].used = 1;
        if (address + 1 > map->size) {
            map->size = address + 1;
        }
    }
}

/* בדיקת אם כתובת בשימוש */
int memory_map_is_used(const MemoryMap *map, size_t address) {
    if (address < MEMORY_SIZE) {
        return map->cells[address].used;
    }
    return 0;
}

/* הדפסת מפה */
void memory_map_print(const MemoryMap *map) {
    size_t i;
    printf("Memory Map (used cells):\n");
    for (i = 0; i < map->size; ++i) {
        if (map->cells[i].used) {
            printf("%03lu: %04x\n", (unsigned long)i, map->cells[i].value);
        }
    }
}
