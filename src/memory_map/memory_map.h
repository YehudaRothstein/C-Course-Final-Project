#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stddef.h>
#include <stdint.h>

#define MEMORY_SIZE 256


typedef struct {
    uint16_t value; 
    int used;       
} MemoryCell;


typedef struct {
    MemoryCell cells[MEMORY_SIZE];
    size_t size; 
} MemoryMap;


void memory_map_init(MemoryMap *map);


void memory_map_set(MemoryMap *map, size_t address, uint16_t value);


uint16_t memory_map_get(const MemoryMap *map, size_t address);


void memory_map_mark_used(MemoryMap *map, size_t address);


int memory_map_is_used(const MemoryMap *map, size_t address);


void memory_map_print(const MemoryMap *map);

#endif
