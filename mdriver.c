#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mm.h"
#include "memlib.h"

#define MAX_ALLOCS 10000

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    mem_init();
    
    if (mm_init() < 0) {
        return 1;
    }
    
    mem_deinit();
    return 0;
}
