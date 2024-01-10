#include <stddef.h>

#ifndef dlmall
#define dlmall

void *dalloc(size_t request);
void dfree(void *memory);
void sanity(void);
void print_freelist(void);
void print_freelist_less(void);
void print_memory_usage(void);

#endif