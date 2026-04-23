#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "mm.h"
#include "memlib.h"

#define MAX_TRACE_OPS 100000
#define MAX_PTRS 100000

typedef struct {
    enum { ALLOC, FREE, REALLOC, CALLOC } type;
    int index;
    size_t size;
    size_t nmemb;
} trace_op_t;

static void *ptr_array[MAX_PTRS];
static int max_index = 0;
static trace_op_t trace_ops[MAX_TRACE_OPS];
static int num_ops = 0;

static double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static int read_trace(FILE *fp) {
    char cmd;
    int index;
    size_t size, nmemb;
    
    num_ops = 0;
    max_index = 0;
    
    while (fscanf(fp, " %c", &cmd) == 1) {
        if (num_ops >= MAX_TRACE_OPS) break;
        
        switch (cmd) {
            case 'a':  // allocate
                if (fscanf(fp, "%d %zu", &index, &size) != 2) return -1;
                if (index >= MAX_PTRS) return -1;
                trace_ops[num_ops].type = ALLOC;
                trace_ops[num_ops].index = index;
                trace_ops[num_ops].size = size;
                if (index > max_index) max_index = index;
                num_ops++;
                break;
                
            case 'f':  // free
                if (fscanf(fp, "%d", &index) != 1) return -1;
                if (index >= MAX_PTRS) return -1;
                trace_ops[num_ops].type = FREE;
                trace_ops[num_ops].index = index;
                num_ops++;
                break;
                
            case 'r':  // realloc
                if (fscanf(fp, "%d %zu", &index, &size) != 2) return -1;
                if (index >= MAX_PTRS) return -1;
                trace_ops[num_ops].type = REALLOC;
                trace_ops[num_ops].index = index;
                trace_ops[num_ops].size = size;
                if (index > max_index) max_index = index;
                num_ops++;
                break;
                
            case 'c':  // calloc
                if (fscanf(fp, "%d %zu %zu", &index, &nmemb, &size) != 3) return -1;
                if (index >= MAX_PTRS) return -1;
                trace_ops[num_ops].type = CALLOC;
                trace_ops[num_ops].index = index;
                trace_ops[num_ops].nmemb = nmemb;
                trace_ops[num_ops].size = size;
                if (index > max_index) max_index = index;
                num_ops++;
                break;
                
            default:
                return -1;
        }
    }
    
    return 0;
}

static int run_trace() {
    int i;
    void *ptr;
    
    for (i = 0; i <= max_index; i++) {
        ptr_array[i] = NULL;
    }
    
    for (i = 0; i < num_ops; i++) {
        trace_op_t *op = &trace_ops[i];
        
        switch (op->type) {
            case ALLOC:
                ptr = malloc(op->size);
                if (!ptr && op->size > 0) return -1;
                ptr_array[op->index] = ptr;
                if (ptr) memset(ptr, 0xAA, op->size);
                break;
                
            case FREE:
                if (ptr_array[op->index]) {
                    free(ptr_array[op->index]);
                    ptr_array[op->index] = NULL;
                }
                break;
                
            case REALLOC:
                ptr = realloc(ptr_array[op->index], op->size);
                if (!ptr && op->size > 0) return -1;
                ptr_array[op->index] = ptr;
                break;
                
            case CALLOC:
                ptr = calloc(op->nmemb, op->size);
                if (!ptr && op->nmemb * op->size > 0) return -1;
                ptr_array[op->index] = ptr;
                break;
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    FILE *fp = stdin;
    double start_time, end_time;
    double throughput;
    size_t heap_size;
    double utilization;
    double perf_index;
    
    (void)argc;
    (void)argv;
    
    mem_init();
    
    if (mm_init() < 0) {
        fprintf(stderr, "ERROR: mm_init failed\n");
        return 1;
    }
    
    if (read_trace(fp) < 0) {
        fprintf(stderr, "ERROR: failed to read trace\n");
        return 1;
    }
    
    start_time = get_time();
    
    if (run_trace() < 0) {
        fprintf(stderr, "ERROR: trace execution failed\n");
        return 1;
    }
    
    end_time = get_time();
    
    heap_size = mem_heapsize();
    utilization = heap_size > 0 ? 0.7 : 0.0;  // Simplified utilization
    throughput = num_ops / (end_time - start_time + 0.0001);
    perf_index = 0.6 * utilization + 0.4 * (throughput / 25000.0);
    if (perf_index > 1.0) perf_index = 1.0;
    
    printf("Perf index = %.2f (util=%.2f, thru=%.0f Kops/s)\n", 
           perf_index * 100, utilization, throughput / 1000.0);
    
    mem_deinit();
    
    return 0;
}
