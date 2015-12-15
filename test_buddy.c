/* file: test_buddy.c */

/**
 * Used for testing buddy.c.
 * Use make to compile everything.
 */

#include <stdlib.h>
#include <stdio.h>

#include "buddy.h"

/**
 * Reads a line from stdin and puts is *s buffer.
 *
 * @param s Buffer to put input into.
 * @param length Length of *s
 * @return void
 */
void readline (char *s, int length) 
{
    char c;

    if (!s) {
        return;  //  Ensure s points to something
    }

    /*
     *  Check for over-length, end of file/ input, and end of line/ newline
     */
    while ((--length > 0) && (c = getchar()) != EOF && c != '\n') {
        *s++ = c;     // Append character to buffer
    }
    *s = '\0';       //  Terminate the returned string
}

/**
 * Entry point.
 */
int main(int argc, char **argv)
{
    char line[20];
    char op;
    int arg;
    
    // Setup memory allocator
    if (mem_init(4096, 128) < 0) {
        fprintf(stderr, "mem_init failed\n");
    }

    while (1) {
        
        // Raed in a line
        readline(line, sizeof(line));

        if (*line == '\0'){
            break;  //  Quit if empty string returned
        }
        // Split line into the operation and argument 
        sscanf(line, " %c %d", &op, &arg);
        if (op == 'A') {
            // Allocate specified size.
            void *ptr = mem_alloc(arg);
            if (ptr == NULL) {
                printf("[-1]");
            } else {
                printf("[%d]", ZERO_REL_ADDR(ptr)); // mem_alloc returns an actual address, need to convert this to 0 relative one.
            }
        } else if (op == 'F') {
            // Free memory. Add base address to argument as it expects an actual memory address
            printf("[%d]", mem_free(mi->base_addr + arg));
        }
        // Dump the memory map 
        mem_dump();
        fflush (stdout);
    }
    
    return 0;
}


/* end file: test_buddy.c */
