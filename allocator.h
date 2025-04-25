#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

#define ALIGNMENT 8

// Function declarations
bool myinit(void *heap_start, size_t heap_size);
void *mymalloc(size_t requested_size);
void myfree(void *ptr);
void *myrealloc(void *old_ptr, size_t new_size);
bool validate_heap();
void dump_heap();

#endif // ALLOCATOR_H