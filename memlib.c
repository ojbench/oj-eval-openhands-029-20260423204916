#include "memlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MAX_HEAP (20 * 1024 * 1024)  /* 20 MB */

static char mem_heap_array[MAX_HEAP];
static char *mem_heap;
static char *mem_brk;
static char *mem_max_addr;

void mem_init(void) {
    mem_heap = mem_heap_array;
    mem_brk = mem_heap;
    mem_max_addr = mem_heap + MAX_HEAP;
}

void mem_deinit(void) {
    /* Nothing to do for static array */
}

void *mem_sbrk(int incr) {
    char *old_brk = mem_brk;
    
    if (incr < 0 || (mem_brk + incr) > mem_max_addr) {
        errno = ENOMEM;
        fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
        return (void *)-1;
    }
    mem_brk += incr;
    return (void *)old_brk;
}

void *mem_heap_lo(void) {
    return (void *)mem_heap;
}

void *mem_heap_hi(void) {
    return (void *)(mem_brk - 1);
}

size_t mem_heapsize(void) {
    return (size_t)(mem_brk - mem_heap);
}

size_t mem_pagesize(void) {
    return (size_t)getpagesize();
}
