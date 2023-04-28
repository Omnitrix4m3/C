/**
 * @file
 *
 * Function prototypes and globals for our memory allocator implementation.
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdio.h>

/* -- Helper functions -- */
void *reuse(size_t size);
void *first_fit(size_t size);
void *worst_fit(size_t size);
void *best_fit(size_t size);
void print_memory(void);

void *search(size_t region_size)
void fill(struct mem_block *block, size_t requested_size, size_t block_size, struct mem_block *start)
void *split(void *block, size_t size)

/* -- C Memory API functions -- */
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

/* -- Data Structures -- */

/**
 * Defines metadata structure for both memory 'regions' and 'blocks.' This
 * structure is prefixed before each allocation's data area.
 */
struct mem_block {
    /**
     * Each allocation is given a unique ID number. If an allocation is split in
     * two, then the resulting new block will be given a new ID.
     */
    unsigned long alloc_id;

    /**
     * The name of this memory block. If the user doesn't specify a name for the
     * block, it should be auto-generated based on the allocation ID.
     */
    char name[32];

    /** Size of the memory region */
    size_t size;

    /** Space used; if usage == 0, then the block has been freed. */
    size_t usage;

    /**
     * Pointer to the start of the mapped memory region. This simplifies the
     * process of finding where memory mappings begin.
     */
    struct mem_block *region_start;

    /**
     * If this block is the beginning of a mapped memory region, the region_size
     * member indicates the size of the mapping. In subsequent (split) blocks,
     * this is undefined.
     */
    size_t region_size;

    /** Next block in the chain */
    struct mem_block *next;

    /**
     * "Padding" to make the total size of this struct 100 bytes. This serves no
     * purpose other than to make memory address calculations easier. If you
     * add members to the struct, you should adjust the padding to compensate
     * and keep the total size at 100 bytes; test cases and tooling will assume
     * a 100-byte header.
     */
    char padding[20];
} __attribute__((packed));

#endif
