# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99

# Source files
SOURCES = explicit_final.c explicit_allocator_tests.c
HEADERS = allocator.h
OBJECTS = $(SOURCES:.c=.o)
TARGET = memory_allocator_test

# Rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

explicit_final.o: explicit_final.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

explicit_allocator_tests.o: explicit_allocator_tests.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test