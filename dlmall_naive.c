/* dlmall_naive.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>

typedef struct Head Head;

struct Head {
    uint16_t bfree; // 2 bytes, the status of the block before
    uint16_t bsize; // 2 bytes, the size of the block before
    uint16_t free;  // 2 bytes, the status of this block
    uint16_t size;  // 2 bytes, the size of this block (max 2^16)
    Head *next;  // 8 byte pointer
    Head *prev;  // 8 byte pointer
};

Head *arena = NULL;

Head *flist;

#define TRUE 1
#define FALSE 0

#define HEAD_SIZE    (sizeof(Head))  // 24 bytes

// the minimum size block (not including the header) we will hand out is 8 bytes
#define MIN(size)   (((size) > (8)) ? (size) : (8))

// "If we want to split a block to accommodate a block of 32 bytes it has to be 62 (8 (min block) + 24 (header) + 32) or larger."
#define LIMIT(size) (MIN(0) + HEAD_SIZE + size)

#define MAGIC(memory)   ((Head*)memory - 1)
#define HIDE(block)    (void*)((Head*)block + 1)

#define ALIGN 8

// The whole heap
#define ARENA   (64*1024)

uint16_t mem_tot = 0;
uint16_t mem_overhead = 0;

void print_freelist() {
    Head *curr = flist;
    int i = 0;

    printf("###FLIST###\n");
    while (curr != NULL) {
        printf("Block %d: %p has size %d\n", i, curr, curr->size);
        curr = curr->next;
        i++;
    }
    printf("###END###\n\n");
}

void print_freelist_less() {
    Head *curr = flist;
    int num_blocks = 0;
    int num_smallest = 0;

    while (curr != NULL) {
        if (curr->size == MIN(0)) {
            num_smallest += 1;
        }
        curr = curr->next;
        num_blocks += 1;
    }

    printf("Number of blocks in freelist: %d\n", num_blocks);
    printf("Number of blocks in freelist with smallest size: %d\n", num_smallest);
}

void print_memory_usage() {
    printf("%d\t%d", mem_overhead, mem_tot);
    //printf("Overhead: %dB out of %dB\n", mem_overhead, mem_tot);
}

// Given a pointer to a block, return a pointer to the block after
Head *after(Head *block) {
    return (Head*) ((char*) block + block->size + HEAD_SIZE);
}

// Given a pointer to a block, return a pointer to the block before
Head *before(Head *block) {
    return (Head*) ((char*) block - block->bsize - HEAD_SIZE);
}

// Given a large enough block and a size, split the block in two and return a pointer to
// the second block
Head *split(Head *block, int size) {
    int rsize = block->size - size - HEAD_SIZE;  // remaining size
    block->size = rsize;        // update the size of the given block to no longer include the part we are removing

    Head *splt = after(block);  // since we updated the size above, this will now point to the right spot in memory
    splt->bsize = block->size;
    splt->bfree = block->free;
    splt->size = size;
    splt->free = FALSE;

    Head *aft = after(splt);
    aft->bsize = splt->size;
    aft->bfree = splt->free;

    return splt;
}


Head *new() {
    if (arena != NULL) {
        printf("ERROR: An arena has already been allocated\n");
        return NULL;
    }

    Head *new = mmap(
        NULL,
        ARENA,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
        );
    
    if (new == MAP_FAILED) {
        printf("mmap failed: error %d\n", errno);
    }

    // make room for own head and dummy/sentinel head
    uint size = ARENA - 2*HEAD_SIZE;

    new->bfree = FALSE;
    new->bsize = 0;
    new->free = TRUE;
    new->size = size;

    Head *sentinel = after(new);

    // Only touch the status fields...
    sentinel->bfree = new->free;
    sentinel->bsize = new->size;
    sentinel->free = FALSE;
    sentinel->size = 0;

    // This is the only arena we have
    arena = (Head*) new;
    return new;
}

void detach(Head *block) {
    // if the block is the first element in flist...
    if (block == flist) {
        // ...just step the flist pointer forward
        if (flist->next != NULL) {
            flist = flist->next;
        } else {
            flist = NULL;
        }
    // if the block is the last element in flist...
    } else if (block->next == NULL) {
        // ...just cut off the previous block's link to this block
        block->prev->next = NULL;
    } else {
        //if we end up here, block is not the first nor the last element in flist
        // reconnect the previous block to the next block
        block->prev->next = block->next;
        // reconnect the next block to the previous block
        block->next->prev = block->prev;
    }
    // no matter where the block was, we need to clear its pointers
    block->next = NULL;
    block->prev = NULL;
}

// insert a block to the front of the flist
void insert(Head *block) {
    // if the flist is empty, this inserted block is the only element
    if (flist == NULL) {
        flist = block;
        return;
    }

    // if the flist isn't empty, set the block to be the first element...
    block->next = flist;
    // ...connect the old first element to the new block...
    block->next->prev = block;
    // ...and finally, make sure the flist pointer points to this new first element
    flist = block;
}
// Adjust the request so that it is aligned to 8 bytes, yet is still at least the requested size
int adjust(size_t request) {
    int res = request;

    int rem = res % ALIGN;
    if (rem != 0) {
        res += (ALIGN - rem);
    }
    
    return MIN(res);
    //return (request < 8) ? (int) MIN(request) : (int) MIN(request) + (MIN(0) - request % MIN(0));
}

// Find the first block that is big enough according to the adjust function
Head *find(int size) {
    if (size <= 0) {
        printf("find: illegal size\n");
        return NULL;
    }
    
    Head *curr = flist;
    Head *found;

    while (curr != NULL) {
        if (curr->size >= size) {
            // this one is big enough to use
            detach(curr);

            if (curr->size > LIMIT(size)) {
                // it's even big enough to split!
                found = split(curr, size);
                insert(curr);
            } else {
                found = curr;
            }

            found->free = FALSE;
            Head *aft = after(found);
            aft->bfree = FALSE;
            
            return found;
        }
        curr = curr->next;
    }

    printf("find failed\n");
    return NULL;
}

void *dalloc(size_t request) {
    if (request <= 0) {
        printf("Invalid request size, must be bigger than 0.\n");
        return NULL;
    }

    if (arena == NULL) {
        flist = new();
    }

    int size = adjust(request);
    Head *taken = find(size);
    
    if (taken == NULL) {
        return NULL;
    } else {
        mem_tot += HEAD_SIZE + taken->size;
        mem_overhead += HEAD_SIZE;
        return HIDE(taken);
    }
}

Head *merge(Head *block) {
    Head *aft = after(block);
    Head *bef = before(block);
    if (block->bfree) {
        // the block before this in memory is free, so we merge them
        detach(bef);
        
        size_t new_size = block->size + HEAD_SIZE + block->bsize;
        block = bef;
        block->size = new_size;
    }

    if (aft->free) {
        detach(aft);
        block->size += HEAD_SIZE + aft->size;
        block->free = TRUE;

        Head *after_aft = after(aft);
        if(after_aft != NULL){
            after_aft->bsize = block->size;
            after_aft->bfree = TRUE;
        }
    }

    return block;
}

void dfree(void *memory) {
    if (memory != NULL) {
        Head *block = (Head*) MAGIC(memory);

        block = merge(block);

        Head *aft = after(block);
        block->free = TRUE;
        aft->bfree = TRUE;

        mem_tot -= (HEAD_SIZE + block->size);
        mem_overhead -= HEAD_SIZE;
        insert(block);
    }
    return;
}

void sanity() {

    if (arena == NULL || flist == NULL) {
        return;
    }

    Head *curr = flist;

    while (curr->next != NULL) {
        if (curr->next->prev != curr) {
            printf("SANITY ERROR: curr->next->prev != curr\n");
        }

        if (curr->free != TRUE) {
            printf("SANITY ERROR: curr->free != TRUE\n");
        }
        curr = curr->next;
    }

    curr = arena;
    Head *aft = after(curr);
    while (aft->size != 0) {
        if (curr->free != aft->bfree) {
            printf("SANITY ERROR: curr->free != aft->bfree\n");
            printf("curr: %p, free: %d\naft: %p, bfree: %d\n", curr, curr->free, aft, aft->bfree);
        }

        if (curr->size != aft->bsize) {
            printf("SANITY ERROR:curr->size != aft->bsize\n");
        }

        curr = aft;
        aft = after(aft);
    }

}