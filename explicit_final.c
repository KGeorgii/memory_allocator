#include "allocator.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/*
 * This program implements a simple explicit memory allocator.
 * The allocator uses a free list to manage free memory blocks and 
 * supports basic operations like initialization, allocation, deallocation, 
 * reallocation, and heap validation.
 */

// Header struct to store block size and allocation status
typedef struct header {
    size_t size;
    bool allocated;
} header;

// Memory block struct that includes a header and pointers for the free list
typedef struct memory_block {
    header hdr;
    struct memory_block *prev;
    struct memory_block *next;
} memory_block;

// Global variables
static void *start;            // Start of the heap segment
static size_t size;            // Size of the heap segment
static char *end;              // End of the heap segment
static memory_block *first_free_block; // Pointer to the first free block in the free list

/*
 * Function: roundup
 * -----------------
 * Rounds up the given size to the nearest multiple of the given alignment.
 * The alignment must be a power of 2.
 */
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

/*
 * Function: split_block_if_poss
 * -----------------------------
 * Splits a block if it is large enough to host an allocation and another free block.
 * The rightmost part becomes a new free block.
 */
void split_block_if_poss(memory_block *cur_block, size_t needed) {
    if (cur_block->hdr.size - needed >= sizeof(header) * 3) {
        size_t remaining = cur_block->hdr.size;
                
        // Update current block while maintaining status in left block
        cur_block->hdr.size = needed;
        cur_block->hdr.allocated = true;

        // Create the new free block
        memory_block *cut_block = (memory_block *)((char *)cur_block + sizeof(header) + needed);

        // Update new free block's size and status
        cut_block->hdr.size = remaining - needed - sizeof(header);
        cut_block->hdr.allocated = false;

        // Add the new free block to the free list
        cut_block->next = first_free_block;
        cut_block->prev = NULL;

        // If the free list is not empty, update the previous pointer of the first node
        if (first_free_block != NULL) {
            first_free_block->prev = cut_block;
        }

        // Update the head of the free list to the new node
        first_free_block = cut_block;
    }
}

/*
 * Function: coalesce_right
 * ------------------------
 * Coalesces a block with its right neighbor if the right neighbor is free.
 */
void coalesce_right(memory_block *new_block) {
    memory_block *right_neighbor = (memory_block *)((char *)(new_block) + sizeof(header) + new_block->hdr.size);
    
    // Remove right neighbor from the free list
    if (right_neighbor->prev != NULL) {
        right_neighbor->prev->next = right_neighbor->next;
    } else {
        first_free_block = right_neighbor->next;
    }

    if (right_neighbor->next != NULL) {
        right_neighbor->next->prev = right_neighbor->prev;
    }

    // Update the new block's size to include the right neighbor
    new_block->hdr.size += sizeof(header) + right_neighbor->hdr.size;
}

/*
 * Function: myinit
 * ----------------
 * Initializes the heap with a given start address and size.
 * Sets up the initial free block.
 */
bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < (ALIGNMENT * 3)) {
        return false;
    }
    
    first_free_block = heap_start;
    start = heap_start;
    size = heap_size - sizeof(header);

    // Initialize the first free block's header
    first_free_block->hdr.size = size;
    first_free_block->hdr.allocated = false;
    first_free_block->prev = NULL;
    first_free_block->next = NULL;

    end = (char *)heap_start + heap_size;
    
    return true;
}

/*
 * Function: mymalloc
 * ------------------
 * Allocates a block of memory of the requested size.
 * Uses the first-fit strategy and splits the block if necessary.
 */
void *mymalloc(size_t requested_size) {
    if (requested_size == 0) {
        return NULL;
    }

    // Round up the requested size to the nearest alignment
    size_t minimum_allocation = ALIGNMENT * 2;
    size_t needed = (requested_size <= minimum_allocation) ? minimum_allocation : roundup(requested_size, ALIGNMENT);

    memory_block *best_fit = NULL;
    memory_block *cur_block = first_free_block;

    // Iterate through the free list to find the best fit block
    while (cur_block != NULL) {
        if (cur_block->hdr.size >= needed) {
            if (best_fit == NULL || cur_block->hdr.size < best_fit->hdr.size) {
                best_fit = cur_block;
            }
        }
        cur_block = cur_block->next;
    }

    // If no suitable block was found, return NULL
    if (best_fit == NULL) {
        return NULL;
    }

    // Split the block if possible
    split_block_if_poss(best_fit, needed);

    // Remove the newly allocated block from the free list
    if (best_fit->prev != NULL) {
        best_fit->prev->next = best_fit->next;
    } else {
        first_free_block = best_fit->next;
    }

    if (best_fit->next != NULL) {
        best_fit->next->prev = best_fit->prev;
    }

    // Allocate the block by updating its header
    best_fit->hdr.allocated = true;

    // Return a pointer to the allocated memory
    return (char *)(best_fit) + sizeof(header);
}

/*
 * Function: myfree
 * ----------------
 * Frees a previously allocated block and adds it back to the free list.
 * Coalesces with neighboring free blocks if possible.
 */
void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    // Calculate the block's starting address and cast it to memory_block
    memory_block *new_block = (memory_block *)((char *)ptr - sizeof(header));

    // Free the block
    new_block->hdr.allocated = false;

    // Add the new free block to the free list
    new_block->next = first_free_block;
    new_block->prev = NULL;

    if (first_free_block != NULL) {
        first_free_block->prev = new_block;
    }

    first_free_block = new_block;

    // Coalesce with the right neighbor if it is free
    memory_block *right_neighbor = (memory_block *)((char *)new_block + sizeof(header) + new_block->hdr.size);
    while ((void *)right_neighbor != end && !right_neighbor->hdr.allocated) {
        coalesce_right(new_block);
        right_neighbor = (memory_block *)((char *)new_block + sizeof(header) + new_block->hdr.size);
    }
}

/*
 * Function: myrealloc
 * -------------------
 * Reallocates a previously allocated block to a new size.
 * Tries to expand in place or allocates a new block and copies the data.
 */
void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    } 
    
    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }

    // Calculate the needed size, rounding up to the nearest alignment
    size_t minimum_allocation = ALIGNMENT * 2;
    size_t needed = (new_size <= minimum_allocation) ? minimum_allocation : roundup(new_size, ALIGNMENT);

    memory_block *cur_block = (memory_block *)((char *)old_ptr - sizeof(header));

    // In-place reallocation if the current block is large enough
    if (cur_block->hdr.size >= needed) {
        split_block_if_poss(cur_block, needed);
        return old_ptr;
    }

    // Try to expand the current block by coalescing with the right neighbor
    memory_block *right_neighbor = (memory_block *)((char *)cur_block + sizeof(header) + cur_block->hdr.size);
    while ((void *)right_neighbor != end && !right_neighbor->hdr.allocated) {
        coalesce_right(cur_block);
        if (cur_block->hdr.size >= needed) {
            split_block_if_poss(cur_block, needed);
            return old_ptr;
        }
        right_neighbor = (memory_block *)((char *)cur_block + sizeof(header) + cur_block->hdr.size);
    }

    // Allocate new memory and copy the old data to the new block
    void *new_ptr = mymalloc(new_size);
    if (new_ptr == NULL) {
        return NULL; // Allocation failed
    }
    memcpy(new_ptr, old_ptr, cur_block->hdr.size);
    myfree(old_ptr);
    
    return new_ptr;
}

/*
 * Function: validate_heap
 * -----------------------
 * Validates the heap's integrity by checking alignment, memory usage, and free block consistency.
 */
bool validate_heap() {
    memory_block *cur_block = (memory_block *)start;
    size_t total_memory = 0;
    size_t free_block_count = 0;

    while ((char *)cur_block < (char *)end) {
        header *cur_header = &(cur_block->hdr);

        // Check if the block size is properly aligned
        if ((cur_header->size & (ALIGNMENT - 1)) != 0) {
            printf("Block size is not aligned.\n");
            return false;
        }

        // Increment total memory to account for the current block
        total_memory += sizeof(header) + cur_header->size;

        // Check for proper alignment of the next block header
        if ((total_memory & (ALIGNMENT - 1)) != 0) {
            printf("Block is misaligned.\n");
            return false;
        }

        // Check for free block consistency
        if (!cur_header->allocated) {
            free_block_count++;
        }

        // Move to the next block
        cur_block = (memory_block *)((char *)cur_block + sizeof(header) + cur_header->size);
    }

    // Check if the total memory used matches the expected heap size
    if ((char *)cur_block != (char *)end) {
        printf("Used memory is more than the heap.\n");
        return false;
    }

    // Check if the number of free blocks matches the dynamically counted free blocks
    size_t actual_free_block_count = 0;
    memory_block *current = first_free_block;
    while (current != NULL) {
        actual_free_block_count++;
        current = current->next;
    }
    
    if (free_block_count != actual_free_block_count) {
        printf("Free block count mismatch. Expected %zu but found %zu.\n", free_block_count, actual_free_block_count);
        return false;
    }

    return true;
}

/*
 * Function: dump_heap
 * -------------------
 * Traverses the heap and prints information about each block.
 * Includes information about free block pointers to help visualize the free list.
 */
void dump_heap() {
    memory_block *cur_block = (memory_block *)start;
    printf("Heap segment starts at address %p, ends at %p.\n", start, end);

    while ((char *)cur_block < (char *)end) {
        printf("Block at: %p\n", cur_block);
        printf("Block size: %lu\n", cur_block->hdr.size);
        printf("Block allocated? %s\n", cur_block->hdr.allocated ? "Yes" : "No");
        
        if (!cur_block->hdr.allocated) {
            printf("Previous free block: %p\n", cur_block->prev);
            printf("Next free block: %p\n", cur_block->next);
        }

        // Move to the next block
        cur_block = (memory_block *)((char *)cur_block + sizeof(header) + cur_block->hdr.size);
    }

    // Calculate the memory used
    size_t nused = 0;
    cur_block = (memory_block *)start;
    while ((char *)cur_block < (char *)end) {
        if (cur_block->hdr.allocated) {
            nused += sizeof(header) + cur_block->hdr.size;
        }
        cur_block = (memory_block *)((char *)cur_block + sizeof(header) + cur_block->hdr.size);
    }

    printf("Memory used: %lu\n", nused);
    printf("Heap size: %lu\n", size);
}
