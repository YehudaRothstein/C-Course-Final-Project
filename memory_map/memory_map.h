#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stddef.h>
#include <stdint.h>

#define MEMORY_SIZE 256

// Memory cell structure (10 bits)
typedef struct {
    uint16_t value; // Only 10 bits used
    int used;       // 1 if used, 0 if free
} MemoryCell;

// Memory map structure
typedef struct {
    MemoryCell cells[MEMORY_SIZE];
    size_t size; // Number of used cells
} MemoryMap;

// Initialize the memory map
void memory_map_init(MemoryMap *map);

// Set a value in the memory map at address
void memory_map_set(MemoryMap *map, size_t address, uint16_t value);

// Get a value from the memory map at address
uint16_t memory_map_get(const MemoryMap *map, size_t address);

// Mark a cell as used
void memory_map_mark_used(MemoryMap *map, size_t address);

// Check if a cell is used
int memory_map_is_used(const MemoryMap *map, size_t address);

// Print the memory map (for debugging)
void memory_map_print(const MemoryMap *map);

#endif // MEMORY_MAP_H
