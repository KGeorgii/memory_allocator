# Custom Memory Allocator

This project implements a custom explicit memory allocator in C. The allocator uses a free list to manage free memory blocks and supports basic memory management operations.

## Features

- Memory initialization with configurable heap size
- Memory allocation with first-fit strategy
- Memory deallocation with right-coalescing
- Memory reallocation with in-place expansion when possible
- Block splitting to minimize fragmentation
- Heap validation for integrity checks
- Memory alignment to 8-byte boundaries

## Implementation Details

The allocator uses explicit free lists to track available memory blocks. Each block includes:

- A header with size and allocation status
- Previous and next pointers for free blocks only

Key functions:
- `myinit`: Initialize the heap memory
- `mymalloc`: Allocate memory of requested size
- `myfree`: Free previously allocated memory
- `myrealloc`: Resize previously allocated memory
- `validate_heap`: Check heap integrity
- `dump_heap`: Print heap information for debugging

## Building and Testing

To build the project:

```bash
make
```

To run the tests:

```bash
make test
```

To clean up build files:

```bash
make clean
```

## Test Suite

The comprehensive test suite includes:

- Basic allocation and deallocation tests
- Memory alignment checks
- Coalescing tests
- Reallocation tests
- Edge case handling
- Fragmentation tests
- Stress testing with random operations
