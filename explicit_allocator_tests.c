#include "allocator.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Test helper functions
void test_init();
void test_malloc_basic();
void test_malloc_alignment();
void test_malloc_out_of_memory();
void test_free_basic();
void test_free_coalesce();
void test_realloc_basic();
void test_realloc_expand();
void test_realloc_edge_cases();
void test_validate_heap();
void test_mixed_operations();
void test_fragmentation();
void stress_test();

// Global variables for testing
#define HEAP_SIZE (1024 * 1024)  // 1MB heap
void *test_heap;

int main() {
    // Initialize random seed
    srand(time(NULL));
    
    // Allocate memory for our test heap
    test_heap = malloc(HEAP_SIZE);
    if (test_heap == NULL) {
        printf("Failed to allocate memory for test heap\n");
        return 1;
    }
    
    // Run tests
    test_init();
    test_malloc_basic();
    test_malloc_alignment();
    test_malloc_out_of_memory();
    test_free_basic();
    test_free_coalesce();
    test_realloc_basic();
    test_realloc_expand();
    test_realloc_edge_cases();
    test_validate_heap();
    test_mixed_operations();
    test_fragmentation();
    stress_test();
    
    // Clean up
    free(test_heap);
    
    printf("All tests passed!\n");
    return 0;
}

void reset_heap() {
    // Reset the test heap before each test
    memset(test_heap, 0, HEAP_SIZE);
    assert(myinit(test_heap, HEAP_SIZE));
}

void test_init() {
    printf("Testing initialization...\n");
    
    // Test successful initialization
    reset_heap();
    assert(validate_heap());
    
    // Test initialization with insufficient size
    assert(!myinit(test_heap, 8));  // Too small
    
    printf("Initialization tests passed!\n");
}

void test_malloc_basic() {
    printf("Testing basic malloc functionality...\n");
    
    reset_heap();
    
    // Allocate memory blocks of different sizes
    void *ptr1 = mymalloc(8);
    assert(ptr1 != NULL);
    assert(validate_heap());
    
    void *ptr2 = mymalloc(64);
    assert(ptr2 != NULL);
    assert(validate_heap());
    
    void *ptr3 = mymalloc(256);
    assert(ptr3 != NULL);
    assert(validate_heap());
    
    // Write to memory to ensure it's usable
    memset(ptr1, 1, 8);
    memset(ptr2, 2, 64);
    memset(ptr3, 3, 256);
    
    // Verify the data in each block
    for (int i = 0; i < 8; i++) assert(((char*)ptr1)[i] == 1);
    for (int i = 0; i < 64; i++) assert(((char*)ptr2)[i] == 2);
    for (int i = 0; i < 256; i++) assert(((char*)ptr3)[i] == 3);
    
    // Free the memory
    myfree(ptr1);
    myfree(ptr2);
    myfree(ptr3);
    assert(validate_heap());
    
    printf("Basic malloc tests passed!\n");
}

void test_malloc_alignment() {
    printf("Testing malloc alignment...\n");
    
    reset_heap();
    
    // Test alignment by allocating memory and checking addresses
    void *ptr1 = mymalloc(1);  // Should be rounded up to minimum allocation
    assert(ptr1 != NULL);
    assert(((size_t)ptr1 & (ALIGNMENT - 1)) == 0);  // Check if aligned
    
    void *ptr2 = mymalloc(7);  // Should be rounded up to minimum allocation
    assert(ptr2 != NULL);
    assert(((size_t)ptr2 & (ALIGNMENT - 1)) == 0);  // Check if aligned
    
    void *ptr3 = mymalloc(15);  // Should be rounded up to next multiple of ALIGNMENT
    assert(ptr3 != NULL);
    assert(((size_t)ptr3 & (ALIGNMENT - 1)) == 0);  // Check if aligned
    
    // Free the memory
    myfree(ptr1);
    myfree(ptr2);
    myfree(ptr3);
    assert(validate_heap());
    
    printf("Malloc alignment tests passed!\n");
}

void test_malloc_out_of_memory() {
    printf("Testing malloc out of memory handling...\n");
    
    reset_heap();
    
    // Allocate a large block that should fit
    void *ptr1 = mymalloc(HEAP_SIZE / 2);
    assert(ptr1 != NULL);
    
    // Try to allocate more than remaining memory
    void *ptr2 = mymalloc(HEAP_SIZE / 2 + 1);
    assert(ptr2 == NULL);  // Should fail
    
    // Allocate another block that should fit
    void *ptr3 = mymalloc(HEAP_SIZE / 4);
    assert(ptr3 != NULL);
    
    // Free the memory
    myfree(ptr1);
    myfree(ptr3);
    assert(validate_heap());
    
    printf("Malloc out of memory tests passed!\n");
}

void test_free_basic() {
    printf("Testing basic free functionality...\n");
    
    reset_heap();
    
    // Allocate and free a single block
    void *ptr = mymalloc(64);
    assert(ptr != NULL);
    myfree(ptr);
    assert(validate_heap());
    
    // Allocate multiple blocks and free them
    void *ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = mymalloc(64);
        assert(ptrs[i] != NULL);
    }
    
    for (int i = 0; i < 10; i++) {
        myfree(ptrs[i]);
        assert(validate_heap());
    }
    
    // Test freeing NULL (should not crash)
    myfree(NULL);
    assert(validate_heap());
    
    printf("Basic free tests passed!\n");
}

void test_free_coalesce() {
    printf("Testing free coalescing...\n");
    
    reset_heap();
    
    // Allocate three adjacent blocks
    void *ptr1 = mymalloc(64);
    void *ptr2 = mymalloc(64);
    void *ptr3 = mymalloc(64);
    assert(ptr1 != NULL && ptr2 != NULL && ptr3 != NULL);
    
    // Free them in a way that should trigger coalescing
    myfree(ptr2);  // Free middle block
    assert(validate_heap());
    
    myfree(ptr3);  // Free right block, should coalesce with middle
    assert(validate_heap());
    
    myfree(ptr1);  // Free left block, should coalesce with the combined middle-right block
    assert(validate_heap());
    
    // After coalescing, we should be able to allocate a large block
    void *ptr4 = mymalloc(64 * 3);  // Should fit in the coalesced space
    assert(ptr4 != NULL);
    myfree(ptr4);
    assert(validate_heap());
    
    printf("Free coalescing tests passed!\n");
}

void test_realloc_basic() {
    printf("Testing basic realloc functionality...\n");
    
    reset_heap();
    
    // Test realloc(NULL, size) - should behave like malloc
    void *ptr1 = myrealloc(NULL, 64);
    assert(ptr1 != NULL);
    memset(ptr1, 1, 64);
    
    // Test realloc(ptr, 0) - should behave like free
    void *ptr2 = myrealloc(ptr1, 0);
    assert(ptr2 == NULL);
    
    // Allocate and then reallocate to smaller size
    ptr1 = mymalloc(128);
    assert(ptr1 != NULL);
    memset(ptr1, 2, 128);
    
    ptr2 = myrealloc(ptr1, 64);
    assert(ptr2 != NULL);
    // Verify data is preserved
    for (int i = 0; i < 64; i++) assert(((char*)ptr2)[i] == 2);
    
    // Reallocate to the same size
    void *ptr3 = myrealloc(ptr2, 64);
    assert(ptr3 != NULL);
    // Verify data is preserved
    for (int i = 0; i < 64; i++) assert(((char*)ptr3)[i] == 2);
    
    myfree(ptr3);
    assert(validate_heap());
    
    printf("Basic realloc tests passed!\n");
}

void test_realloc_expand() {
    printf("Testing realloc expansion...\n");
    
    reset_heap();
    
    // Allocate a block
    void *ptr1 = mymalloc(64);
    assert(ptr1 != NULL);
    memset(ptr1, 3, 64);
    
    // Allocate another block after it
    void *ptr2 = mymalloc(64);
    assert(ptr2 != NULL);
    memset(ptr2, 4, 64);
    
    // Free the second block to allow expansion
    myfree(ptr2);
    
    // Now reallocate the first block to a larger size
    void *ptr3 = myrealloc(ptr1, 128);
    assert(ptr3 != NULL);
    
    // Verify that data is preserved
    for (int i = 0; i < 64; i++) assert(((char*)ptr3)[i] == 3);
    
    // Free the expanded block
    myfree(ptr3);
    assert(validate_heap());
    
    printf("Realloc expansion tests passed!\n");
}

void test_realloc_edge_cases() {
    printf("Testing realloc edge cases...\n");
    
    reset_heap();
    
    // Test reallocating to a size that doesn't fit in place and requires copying
    void *ptr1 = mymalloc(64);
    void *ptr2 = mymalloc(256);  // Prevents expansion of ptr1
    void *ptr3 = mymalloc(64);
    
    assert(ptr1 != NULL && ptr2 != NULL && ptr3 != NULL);
    
    memset(ptr1, 5, 64);
    memset(ptr2, 6, 256);
    memset(ptr3, 7, 64);
    
    // Reallocate ptr1 to a size that won't fit in place
    void *ptr4 = myrealloc(ptr1, 128);
    assert(ptr4 != NULL);
    
    // Verify data was correctly copied
    for (int i = 0; i < 64; i++) assert(((char*)ptr4)[i] == 5);
    
    // Clean up
    myfree(ptr2);
    myfree(ptr3);
    myfree(ptr4);
    assert(validate_heap());
    
    printf("Realloc edge case tests passed!\n");
}

void test_validate_heap() {
    printf("Testing heap validation...\n");
    
    reset_heap();
    
    // Allocate and free some memory to create a complex heap state
    void *ptr1 = mymalloc(64);
    void *ptr2 = mymalloc(128);
    void *ptr3 = mymalloc(256);
    
    myfree(ptr2);  // Free the middle block
    
    // Validate the heap
    assert(validate_heap());
    
    // Clean up
    myfree(ptr1);
    myfree(ptr3);
    assert(validate_heap());
    
    printf("Heap validation tests passed!\n");
}

void test_mixed_operations() {
    printf("Testing mixed memory operations...\n");
    
    reset_heap();
    
    // Perform a mix of malloc, free, and realloc operations
    void *ptr1 = mymalloc(64);
    void *ptr2 = mymalloc(128);
    
    myfree(ptr1);
    
    void *ptr3 = myrealloc(ptr2, 64);  // Shrink
    void *ptr4 = mymalloc(32);
    void *ptr5 = mymalloc(16);
    
    myfree(ptr4);
    
    void *ptr6 = myrealloc(ptr3, 96);  // Expand
    void *ptr7 = myrealloc(ptr5, 48);  // Expand
    
    // Clean up
    myfree(ptr6);
    myfree(ptr7);
    assert(validate_heap());
    
    printf("Mixed operations tests passed!\n");
}

void test_fragmentation() {
    printf("Testing memory fragmentation handling...\n");
    
    reset_heap();
    
    // Create a fragmented heap by allocating and freeing in a specific pattern
    void *ptrs[20];
    
    // Allocate 20 blocks
    for (int i = 0; i < 20; i++) {
        ptrs[i] = mymalloc(32);
        assert(ptrs[i] != NULL);
    }
    
    // Free every other block to create fragmentation
    for (int i = 0; i < 20; i += 2) {
        myfree(ptrs[i]);
    }
    
    // Validate the heap
    assert(validate_heap());
    
    // Try to allocate a block that should fit in one of the fragments
    void *ptr = mymalloc(32);
    assert(ptr != NULL);
    
    // Try to allocate a block that won't fit in any single fragment
    void *large_ptr = mymalloc(64);
    
    // Clean up
    myfree(ptr);
    if (large_ptr != NULL) myfree(large_ptr);
    
    for (int i = 1; i < 20; i += 2) {
        myfree(ptrs[i]);
    }
    
    assert(validate_heap());
    
    printf("Fragmentation tests passed!\n");
}

void stress_test() {
    printf("Running stress test...\n");
    
    reset_heap();
    
    #define MAX_ALLOCATIONS 1000
    #define MAX_ALLOC_SIZE 1024
    
    void *ptrs[MAX_ALLOCATIONS] = {NULL};
    
    // Perform random allocations, reallocations, and frees
    for (int i = 0; i < 5000; i++) {
        int operation = rand() % 3;  // 0: malloc, 1: realloc, 2: free
        int index = rand() % MAX_ALLOCATIONS;
        
        switch (operation) {
            case 0: // malloc
                if (ptrs[index] == NULL) {
                    size_t size = rand() % MAX_ALLOC_SIZE + 1;
                    ptrs[index] = mymalloc(size);
                    if (ptrs[index] != NULL) {
                        memset(ptrs[index], index & 0xFF, size);
                    }
                }
                break;
                
            case 1: // realloc
                if (ptrs[index] != NULL) {
                    size_t size = rand() % MAX_ALLOC_SIZE + 1;
                    void *new_ptr = myrealloc(ptrs[index], size);
                    if (new_ptr != NULL) {
                        ptrs[index] = new_ptr;
                    }
                }
                break;
                
            case 2: // free
                if (ptrs[index] != NULL) {
                    myfree(ptrs[index]);
                    ptrs[index] = NULL;
                }
                break;
        }
        
        // Periodically validate the heap
        if (i % 500 == 0) {
            assert(validate_heap());
        }
    }
    
    // Clean up any remaining allocations
    for (int i = 0; i < MAX_ALLOCATIONS; i++) {
        if (ptrs[i] != NULL) {
            myfree(ptrs[i]);
        }
    }
    
    assert(validate_heap());
    
    printf("Stress test passed!\n");
}