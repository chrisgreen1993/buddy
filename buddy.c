/* file: buddy.c */

/**
 * To compile simply run the make command.
 * test_buddy wraps this and allows you to enter the appropriate operations.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "buddy.h"

/*
 * Macros for bitmaps - setting, clearing and testing if set.
 */
#define BITS 8
#define BIT_SET(map, bit) (map[(bit) / BITS] |= (0x80 >> ((bit) % BITS)))
#define BIT_CLEAR(map, bit) (map[(bit) / BITS] &= ~(0x80 >> ((bit) % BITS)))
#define BIT_ISSET(map, bit) (map[(bit) / BITS] & (0x80 >> ((bit) % BITS)))

/**
 * Calculates log base 2 of a power of 2
 * 
 * @author From http://graphics.stanford.edu/~seander/bithacks.html
 * @param pow_2 value to calculate
 * @return unsigned int log base 2 of param pow_2
 */
static unsigned int log_2(unsigned int pow_2)
{
    int i;
    static const unsigned int b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 
                                    0xFF00FF00, 0xFFFF0000};
    register unsigned int r = (pow_2 & b[0]) != 0;
    for (i = 4; i > 0; i--) {
        r |= ((pow_2 & b[i]) != 0) << i;
    }
    return r;
}

/**
 * Rounds up to next highest power of 2
 * 
 * @author From http://graphics.stanford.edu/~seander/bithacks.html
 * @param n value to round up
 * @return unsigned int power of 2
 */
static unsigned int upper_pow_2(unsigned int n) 
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

/**
 * Gets the bitmap location of the last bit of a block.
 *
 * @author Chris Green
 * @param mem_block Memory block
 * @param order Teh order of mem_block
 * @return int Location in bitmap of last bit
 */
static int get_last_bit(struct mem_block *mem_block, unsigned int order)
{
    return ((ZERO_REL_ADDR(mem_block) + (1 << order)) / mi->min_size) - 1;
}


/**
 * Sets last bit of block in bitmap to 1 to represent allocated.
 *
 * @author Chris Green
 * @param mem_block Memory block
 * @param order The order of mem_block
 * @return void
 */
static void set_allocated(struct mem_block *mem_block, unsigned int order)
{
    BIT_SET(mi->bitmap, get_last_bit(mem_block, order));
}

/**
 * Sets last bit of block in bitmap to 0 to represent free.
 *
 * @author Chris Green
 * @param mem_block Memory block
 * @param order The order of mem_block
 * @return void
 */
static void set_free(struct mem_block *mem_block, unsigned int order)
{
    BIT_CLEAR(mi->bitmap, get_last_bit(mem_block, order));
}

/**
 * Checks to see if a block is allocated in the bitmap.
 * Looks at the last bit of the block.
 *
 * @author Chris Green
 * @param mem_block Memory block
 * @param order The order of mem_block
 * @return int True = 1, False = 0
 */
static int is_allocated(struct mem_block *mem_block, unsigned int order)
{
    if (BIT_ISSET(mi->bitmap, get_last_bit(mem_block, order))) {
        return 1;
    }
    return 0;
}

/**
 * Gets the next offset in the bitmap. Recursive.
 * Works by starting at offset 0 from start bit, then 1, 3, 7 etc. 
 * When we hit a set bit, we have found the size of our block as when we set a block as allocated, we ony set the last bit.
 *
 * @author Chris Green
 * @param start The starting bit.
 * @param n Next offset to calculate.
 * @return int End bit of the block.
 */
static int next_offset(int start, int n)
{
    // Make sure we aren't at the end of the bitmap.
    if (start + n >= (mi->total_size / mi->min_size)) {
        return -1;
    }
    if (BIT_ISSET(mi->bitmap, start + n)) {
        return start + n;
    }
    n = (2 * n) + 1; // Work out next offset
    return next_offset(start, n);
}

/**
 * Used to get the order of a block we only have a pointer to.
 * Calls next_offset which does most of the work.
 *
 * @author Chris Green
 * @param mem_block Memory block
 * @return int The order of the block.
 */
static int get_block_order(struct mem_block *mem_block)
{
    int start;
    int end;
    
    start = ZERO_REL_ADDR(mem_block) / mi->min_size; // Get start bit in bitmap
    end = next_offset(start, 0); // Find the end
    if (end < 0) {
        return -1;
    }
    return log_2((end - start + 1) * mi->min_size); // Calculate the order
    
}

/**
 * Gets the buddy of the mem_block by XORing with the size
 * 
 * @author Chris Green
 * @param mem_block The block to get the buddy of.
 * @param order The order of mem_block
 * @return Address of buddy
 */
static void *get_buddy(struct mem_block *mem_block, unsigned int order)
{
    unsigned long buddy;
    
    buddy = ZERO_REL_ADDR(mem_block) ^ (1 << order);

    return (void *)(unsigned int)buddy + (unsigned int)mi->base_addr;
}

/**
 * Initialises the buddy memory allocator.
 * Sets max size and minimum block size and puts a block on the highest free list.
 *
 * @author Chris Green
 * @param total_size Total memory size for the allocator.
 * @param min_size The minimum block size.
 * @return int -1 on failure, 0 on ok.
 */
int mem_init(size_t total_size, size_t min_size) 
{
    int i;
    int num_blocks;
    struct mem_block *mem_block;

    // Allocate memory for mem_info struct
    mi = sbrk(sizeof(struct mem_info));
    if (mi < 0) {
        return -1;
    }
    
    // Total size (rounded up to power of 2) and order
    mi->total_size = upper_pow_2(total_size);
    mi->total_order = log_2(mi->total_size);
    // Minimum block size (rounded up to power of 2) and order
    mi->min_size = upper_pow_2(min_size);
    mi->min_order = log_2(mi->min_size);
   
    // Set the base address and expand heap by our total size
    mi->base_addr = sbrk(mi->total_size);
    if (mi->base_addr < 0) {
        return -1;
    }
    
    // Get no of blocks and alloc memory for the bitmap
    num_blocks = mi->total_size / mi->min_size;
    mi->bitmap = sbrk(num_blocks / BITS + 1);
    if (mi->bitmap < 0) {
        return -1;
    }
    
    // Allocate memory for each of the free lists 
    // FIX: currently allocates memory for free lists we don't use
    mi->free_lists = sbrk((mi->total_order + 1) * sizeof(struct list_head));
    if (mi-> free_lists < 0) {
        return -1;
    }
    
    // Initialise linked lists using macro from sys/queue.h
    for (i = mi->min_order; i <= mi->total_order; i++) {
        SLIST_INIT(&mi->free_lists[i]);
    }
    
    // Insert block at the highest free list
    mem_block = (struct mem_block *)mi->base_addr;
    mem_block->order = mi->total_order;
    SLIST_INSERT_HEAD(&mi->free_lists[mi->total_order], mem_block, link);

    return 0; 
}


/**
 * Allocates memory using buddy system.
 * Looks for free blocks in the same size free list.
 * If there aren't any blocks, we keep going up to the next free list until we find a block.
 * Then we split until we have the right size.
 * We also set the block as allocated in the bitmap.
 *
 * @author Chris Green
 * @param size Size to allocate.
 * @return Pointer to memory address.
 */
void *mem_alloc(size_t size) 
{ 
    unsigned int order;
    struct mem_block *mem_block;
    struct mem_block *buddy;
    int i;
    
    if (size > mi->total_size) {
        return NULL;
    }
    if (size < mi->min_size) {
        size = mi->min_size;
    }
    
    // Round up to next power of 2 and get log base 2.
    // This will give us the order, which is also the free list number.
    order = log_2(upper_pow_2(size));
    
    // Loop through free lists.
    for (i = order; i <= mi->total_order; i++) {
        // If the free list is empty we move up an order to the next list
        if (SLIST_EMPTY(&mi->free_lists[i])) {
            continue;
        }
        // Otherwise we get the first block from the list and remove it.
        mem_block = SLIST_FIRST(&mi->free_lists[i]);
        SLIST_REMOVE_HEAD(&mi->free_lists[i], link);
        // If we're on a list higher than the size we're looking for, we need to split the block.
        while (i > order) {
            i--;
            // Get the buddy
            buddy = (struct mem_block *)((unsigned int)mem_block + (1UL << i));
            buddy->order = i;
            // Insert into free list
            SLIST_INSERT_HEAD(&mi->free_lists[i], buddy, link);
        }
        // Set block as allocated and return pointer
        set_allocated(mem_block, i);
        mem_block->order = order;
        return mem_block;
    }
    return NULL;
}

/**
 * Frees given ptr's memory block in the buddy allocator.
 * Gets the addresses order and checks if allocated.
 * If not we can coalesce smaller blocks into larger ones.
 *
 * @author Chris Green
 * @param ptr Pointer to memory address to free.
 * @return -1 on failure, 0 on success.
 */
int mem_free(void *ptr) 
{
    struct mem_block *mem_block;
    struct mem_block *buddy;
    int order;

    if (ptr == NULL) {
        return -1;
    }

    mem_block = (struct mem_block *)ptr;
    // Gets the order of the block so we know how large it is 
    order = get_block_order(mem_block);
    if (order < mi->min_order || order > mi->total_order) {
        return -1;
    }

    while (order < mi->total_order) {
        // Gets blocks buddy
        buddy = (struct mem_block *)get_buddy(mem_block, order);
        // Check if buddy allocated. If it is we don't need to coalesce blocks
        if (is_allocated(buddy, order)) {
            break;
        }
        // Make sure buddy is correct size.
        if (!is_allocated(buddy, order) && (buddy->order != order)) {
            break;
        }
        // Remove buddy from free list.
        SLIST_REMOVE(&mi->free_lists[order], buddy, mem_block, link);
        if (buddy < mem_block) {
            mem_block = buddy; // Swap them around if buddy smaller
        }
        // Set as free in bitmap.
        set_free(mem_block, order);
        order++;
        mem_block->order = order;
    }
    mem_block->order = order;
    // Set as free in bitmap and insert block in appropriate free list.
    set_free(mem_block, order);
    SLIST_INSERT_HEAD(&mi->free_lists[order], mem_block, link);
    
    return 0;
}

/**
 * Dumps the memory layout of the buddy allocator.
 * Goes through each bit in the bitmap, checking if there is a free block or if it is allocated.
 * Bit of a mess but it does the job.
 *
 * @author Chris Green
 * @return void
 */
void mem_dump()
{
    int i; 
    int j;
    int curr_loc = 0;
    struct mem_block *mem_block;
   
    for (i = 0; i <= (mi->total_size / mi->min_size); i++) {
        // If bit is set, it is the final bit in an allocated block.
        // So we must take it away from current location to get size.
        if (BIT_ISSET(mi->bitmap, i)) {
            printf("(%dA)", (i + 1 - curr_loc) * mi->min_size);
            curr_loc = i + 1;
        } else {
            // If the bit isn't set, it could be free, so we must loop through all of the free lists.
            for (j = mi->min_order; j <= mi->total_order; j++) {
                SLIST_FOREACH(mem_block, &mi->free_lists[j], link) {
                    // Check to see if the zero relative address is the same as the address of the current bit.
                    if (ZERO_REL_ADDR(mem_block) == i * mi->min_size) {
                        // Figure out the size and print it, then update the current location.
                        printf("(%dF)", (ZERO_REL_ADDR(mem_block) + (1 << mem_block->order)) - ZERO_REL_ADDR(mem_block));
                        curr_loc = (ZERO_REL_ADDR(mem_block) + (1 << mem_block->order)) / mi->min_size;
                    }
                }
            }
        }
    }
    printf("\n");
}

/* end file: buddy.c */
