/*
 * mm.c - Dynamic Memory Allocator
 * 
 * This implementation uses an explicit free list with boundary tags.
 * 
 * Block Structure:
 * - Allocated blocks: [header | payload | footer]
 * - Free blocks: [header | prev pointer | next pointer | ... | footer]
 * 
 * Header/Footer: 8 bytes containing size and allocated bit
 * - Size is stored in upper bits (aligned to 8 bytes)
 * - Lowest bit is allocation flag (1 = allocated, 0 = free)
 * 
 * Free List Organization:
 * - Doubly-linked explicit free list
 * - LIFO insertion policy for simplicity
 * - Constant time insertion and removal
 * 
 * Allocation Strategy:
 * - First-fit search through free list
 * - Immediate coalescing on free
 * 
 * Alignment: 8 bytes (DSIZE)
 * Minimum block size: 24 bytes (header + prev + next + footer)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

/* Basic constants and macros */
#define WSIZE       8       /* Word and header/footer size (bytes) */
#define DSIZE       16      /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */
#define MIN_BLOCK_SIZE 24   /* Minimum block size */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given free block ptr bp, get/set prev/next pointers in free list */
#define GET_PREV_FREE(bp)  (*(void **)(bp))
#define GET_NEXT_FREE(bp)  (*(void **)((char *)(bp) + WSIZE))
#define SET_PREV_FREE(bp, val)  (*(void **)(bp) = (val))
#define SET_NEXT_FREE(bp, val)  (*(void **)((char *)(bp) + WSIZE) = (val))

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */
static char *free_listp = 0;  /* Pointer to first free block */

/* Function prototypes */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void add_to_free_list(void *bp);
static void remove_from_free_list(void *bp);

/*
 * mm_init - Initialize the memory allocator
 */
int mm_init(void) {
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));  /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));  /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));      /* Epilogue header */
    heap_listp += (2*WSIZE);
    free_listp = NULL;
    
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    
    return 0;
}

/*
 * malloc - Allocate a block with at least size bytes of payload
 */
void *malloc(size_t size) {
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;
    
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;
    
    /* Adjust block size to include overhead and alignment reqs */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * free - Free a block
 */
void free(void *ptr) {
    if (ptr == NULL)
        return;
    
    size_t size = GET_SIZE(HDRP(ptr));
    
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * realloc - Reallocate a block
 */
void *realloc(void *ptr, size_t size) {
    void *newptr;
    size_t copySize;
    
    /* If ptr is NULL, this is just malloc */
    if (ptr == NULL)
        return malloc(size);
    
    /* If size is 0, this is just free */
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    /* Allocate new block */
    newptr = malloc(size);
    if (newptr == NULL)
        return NULL;
    
    /* Copy old data to new block */
    copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, ptr, copySize);
    
    /* Free old block */
    free(ptr);
    
    return newptr;
}

/*
 * calloc - Allocate and zero-initialize a block
 */
void *calloc(size_t nmemb, size_t size) {
    size_t bytes = nmemb * size;
    void *ptr = malloc(bytes);
    
    if (ptr != NULL)
        memset(ptr, 0, bytes);
    
    return ptr;
}

/*
 * mm_checkheap - Check heap consistency
 */
void mm_checkheap(void) {
    char *bp;
    
    /* Check prologue blocks */
    if (GET_SIZE(HDRP(heap_listp)) != DSIZE || !GET_ALLOC(HDRP(heap_listp))) {
        printf("Bad prologue header\n");
    }
    
    /* Check each block */
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        /* Check alignment */
        if ((size_t)bp % DSIZE != 0) {
            printf("Block %p not aligned\n", bp);
        }
        
        /* Check header/footer consistency */
        if (GET(HDRP(bp)) != GET(FTRP(bp))) {
            printf("Header/footer mismatch at %p\n", bp);
        }
        
        /* Check coalescing: no two consecutive free blocks */
        if (!GET_ALLOC(HDRP(bp)) && !GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
            printf("Consecutive free blocks at %p\n", bp);
        }
    }
    
    /* Check free list consistency */
    int free_count_list = 0;
    for (bp = free_listp; bp != NULL; bp = GET_NEXT_FREE(bp)) {
        free_count_list++;
        
        /* Check if block is marked as free */
        if (GET_ALLOC(HDRP(bp))) {
            printf("Allocated block in free list: %p\n", bp);
        }
        
        /* Check if pointers are within heap bounds */
        if (bp < (char *)mem_heap_lo() || bp > (char *)mem_heap_hi()) {
            printf("Free list pointer out of bounds: %p\n", bp);
        }
        
        /* Check prev/next consistency */
        if (GET_NEXT_FREE(bp) != NULL) {
            if (GET_PREV_FREE(GET_NEXT_FREE(bp)) != bp) {
                printf("Free list inconsistency at %p\n", bp);
            }
        }
    }
    
    /* Count free blocks by iterating through heap */
    int free_count_heap = 0;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)))
            free_count_heap++;
    }
    
    /* Check if counts match */
    if (free_count_list != free_count_heap) {
        printf("Free block count mismatch: list=%d, heap=%d\n", 
               free_count_list, free_count_heap);
    }
}

/*
 * extend_heap - Extend heap with free block
 */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;
    
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
    
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * coalesce - Merge adjacent free blocks
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) {            /* Case 1: both allocated */
        add_to_free_list(bp);
        return bp;
    }
    
    else if (prev_alloc && !next_alloc) {      /* Case 2: next is free */
        remove_from_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_to_free_list(bp);
    }
    
    else if (!prev_alloc && next_alloc) {      /* Case 3: prev is free */
        remove_from_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        add_to_free_list(bp);
    }
    
    else {                                     /* Case 4: both free */
        remove_from_free_list(PREV_BLKP(bp));
        remove_from_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        add_to_free_list(bp);
    }
    
    return bp;
}

/*
 * place - Place block of asize bytes at start of free block bp
 */
static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    
    remove_from_free_list(bp);
    
    if ((csize - asize) >= (2*DSIZE)) {
        /* Split the block */
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        add_to_free_list(bp);
    }
    else {
        /* Don't split, use entire block */
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * find_fit - Find a fit for a block with asize bytes
 */
static void *find_fit(size_t asize) {
    void *bp;
    
    /* First-fit search */
    for (bp = free_listp; bp != NULL; bp = GET_NEXT_FREE(bp)) {
        if (asize <= GET_SIZE(HDRP(bp))) {
            return bp;
        }
    }
    return NULL; /* No fit */
}

/*
 * add_to_free_list - Add block to free list (LIFO)
 */
static void add_to_free_list(void *bp) {
    SET_NEXT_FREE(bp, free_listp);
    SET_PREV_FREE(bp, NULL);
    
    if (free_listp != NULL) {
        SET_PREV_FREE(free_listp, bp);
    }
    
    free_listp = bp;
}

/*
 * remove_from_free_list - Remove block from free list
 */
static void remove_from_free_list(void *bp) {
    void *prev = GET_PREV_FREE(bp);
    void *next = GET_NEXT_FREE(bp);
    
    if (prev == NULL) {
        /* bp is first block in free list */
        free_listp = next;
    } else {
        SET_NEXT_FREE(prev, next);
    }
    
    if (next != NULL) {
        SET_PREV_FREE(next, prev);
    }
}
