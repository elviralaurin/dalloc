/* test.c */

#include "dlmall.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#define LOOPS 10
#define NBLOCKS 10

void benchmark(int min_size, int max_size) {
    // What is happening with the length of the freelist as we do more malloc/free ops?
    // What is the size of the blocks in the freelist?
    int iters = LOOPS;
    int num_blocks = NBLOCKS;
    int *blocks[num_blocks];

    for (int i = 0; i < iters; i++) {
        printf("\n~~~* Iteration %d *~~~\n\n", i + 1);
        for (int b = 0; b < num_blocks; b++) {
            // Request a random size block with dalloc, put the pointer in blocks[b]
            int req_size = (rand() % (max_size - min_size + 1)) + min_size;

            blocks[b] = dalloc(req_size);
            
            if (blocks[b] == NULL) {
                printf("dalloc failed\n");
                return;
            }
            // store some data in the block
            *blocks[b] = 123;
            sanity();
        }

        for (int b = 0; b < num_blocks; b++) {
            dfree(blocks[b]);
            sanity();
        }
        //printFreelist();
        print_freelist_less();
        sanity();
    }
}

void benchmark_xl() {
    int iters = 100000;
    int num_blocks = 1000;
    int *blocks[num_blocks];

    for (int i = 0; i < iters; i++) {
        //printf("\nIteration %d\n", i + 1);
        for (int b = 0; b < num_blocks; b++) {
            // Request a random size block with dalloc, put the pointer in blocks[b]
            int req_size = 16;

            blocks[b] = dalloc(req_size);
            
            if (blocks[b] == NULL) {
                printf("dalloc failed\n");
                return;
            }
            // store some data in the block
            *blocks[b] = 123;
            //sanity();
        }
        //print_memory_usage();

        for (int b = 0; b < num_blocks; b++) {
            dfree(blocks[b]);
            //sanity();
        }
        //print_memory_usage();
        //printFreelist();
        //sanity();
        //sanity();
    }
}

int benchmark_size() {
    int num_blocks = 10;
    int max_req_size = 64;
    int *blocks[num_blocks];

    for (int req_size = 1; req_size <= max_req_size; req_size++) {
        for (int b = 0; b < num_blocks; b++) {
            blocks[b] = dalloc(req_size);
            *blocks[b] = 123;
        }

        printf("%d\t", req_size);
        print_memory_usage();
        printf("\n");

        for (int b = 0; b < num_blocks; b++) {
            dfree(blocks[b]);
        }
    }
}

int main(int argc, char *argv[]) {
    sanity();
    printf("\nRUNNING BENCHMARK...\n");
    //benchmark(1, 10);'
    //benchmark_xl();
    int *block = dalloc(32);
    print_memory_usage();
    *block = 123;
    dfree(block);
    printf("\nBENCHMARK COMPLETED.\n");

    sanity();
    //void *mem = dalloc(sizeof(int));

    //sanity();

    //dfree(mem);
    //sanity();

    return 0;
}