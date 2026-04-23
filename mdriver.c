#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mm.h"
#include "memlib.h"

#define MAX_ALLOCS 10000

int main(int argc, char *argv[]) {
    void *ptrs[MAX_ALLOCS];
    int i;
    
    (void)argc;
    (void)argv;
    
    printf("Testing malloc implementation...\n");
    
    mem_init();
    
    if (mm_init() < 0) {
        fprintf(stderr, "mm_init failed\n");
        return 1;
    }
    
    printf("Test 1: Basic allocation and deallocation\n");
    for (i = 0; i < 10; i++) {
        ptrs[i] = malloc(100);
        if (ptrs[i] == NULL) {
            fprintf(stderr, "malloc failed at iteration %d\n", i);
            return 1;
        }
        sprintf((char *)ptrs[i], "Block %d", i);
    }
    
    for (i = 0; i < 10; i++) {
        free(ptrs[i]);
    }
    printf("Test 1 passed\n");
    
    printf("Test 2: Variable size allocations\n");
    for (i = 0; i < 100; i++) {
        int size = (i * 13 + 17) % 1000 + 1;
        ptrs[i] = malloc(size);
        if (ptrs[i] == NULL) {
            fprintf(stderr, "malloc failed at iteration %d\n", i);
            return 1;
        }
        memset(ptrs[i], i % 256, size);
    }
    
    for (i = 0; i < 100; i++) {
        free(ptrs[i]);
    }
    printf("Test 2 passed\n");
    
    printf("Test 3: Realloc test\n");
    void *p = malloc(100);
    if (p == NULL) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    strcpy((char *)p, "Hello");
    
    p = realloc(p, 200);
    if (p == NULL) {
        fprintf(stderr, "realloc failed\n");
        return 1;
    }
    
    if (strcmp((char *)p, "Hello") != 0) {
        fprintf(stderr, "realloc corrupted data\n");
        return 1;
    }
    free(p);
    printf("Test 3 passed\n");
    
    printf("Test 4: Calloc test\n");
    p = calloc(10, 20);
    if (p == NULL) {
        fprintf(stderr, "calloc failed\n");
        return 1;
    }
    
    char *cp = (char *)p;
    for (i = 0; i < 200; i++) {
        if (cp[i] != 0) {
            fprintf(stderr, "calloc did not zero memory\n");
            return 1;
        }
    }
    free(p);
    printf("Test 4 passed\n");
    
    printf("Test 5: Heap checker test\n");
    mm_checkheap();
    printf("Test 5 passed\n");
    
    printf("\nAll tests passed!\n");
    
    mem_deinit();
    return 0;
}
