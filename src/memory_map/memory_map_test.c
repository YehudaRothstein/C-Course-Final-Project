#include "memory_map.h"
#include <stdio.h>

int main() {
    MemoryMap map;
    memory_map_init(&map);
    memory_map_set(&map, 0, 0x155);
    memory_map_set(&map, 1, 0x2AA);
    memory_map_set(&map, 10, 0x3FF);
    memory_map_print(&map);
    printf("Cell 1: %x\n", memory_map_get(&map, 1));
    printf("Cell 10 used: %d\n", memory_map_is_used(&map, 10));
    printf("Cell 20 used: %d\n", memory_map_is_used(&map, 20));
    return 0;
}
