#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stddef.h>
#include <limits.h>

#define MEMORY_SIZE 256

typedef unsigned short word_t; 

typedef struct {
    word_t value;  /* 10 ביטים */
    int used;      /* דגל בוליאני */
} MemoryCell;


typedef struct {
    MemoryCell cells[MEMORY_SIZE];
    size_t size; /* האינדקס הגבוה ביותר בשימוש + 1 */
} MemoryMap;


void memory_map_init(MemoryMap *map);


void memory_map_set(MemoryMap *map, size_t address, word_t value);


word_t memory_map_get(const MemoryMap *map, size_t address);


void memory_map_mark_used(MemoryMap *map, size_t address);


int memory_map_is_used(const MemoryMap *map, size_t address);


void memory_map_print(const MemoryMap *map);

#endif
