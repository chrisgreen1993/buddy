/* file: buddy.h */

/* See top of buddy.c for building + usage */

#ifndef BUDDY_DEFINED
#define BUDDY_DEFINED

// We use the macros in queue.h for our linked lists. For more info see man page.
#include <sys/queue.h>

// Calculates teh zero relative address based upon the base address of the buddy memory.
#define ZERO_REL_ADDR(addr) ((unsigned int)addr - (unsigned int)mi->base_addr)

/**
 * Main datastructure for buddy allocator
 */
struct mem_info {
    void *base_addr; // Base address of the memory
    unsigned int total_size; // Total memory size
    unsigned int total_order; // Total order
    unsigned int min_size; // Min block size
    unsigned int min_order; // min block size order
    unsigned int *bitmap; // Pointer to bitmap
    SLIST_HEAD(list_head, mem_block) *free_lists; // Pointer to free lists. Note use of sys/queue.h macros.
};

/**
 * Holds blocks in the free list. Along with order.
 */
struct mem_block {
    SLIST_ENTRY(mem_block) link;
    unsigned int order;
};

struct mem_info *mi; // Global pointer to mem_info, so we don't have to pass it everywhere.

/**
 * Public API functions. See docblocks in .c file for more details.
 */
int mem_init(size_t total_size, size_t min_size);
void *mem_alloc(size_t size);
int mem_free(void *ptr);
void mem_dump();

#endif

/* end file: buddy.h */
