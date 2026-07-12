#include "some-libc.h"
#include "syscall.h"
#include <stddef.h>

#define HEAP_ALIGN 8
#define MAGIC_FREE 0xDEADBEEFu
#define MAGIC_USED 0xCAFEBABEu

typedef struct block {
    int size;
    unsigned int magic;
    struct block* next;
} block_t;

static const int HEADER_SIZE = (int)sizeof(block_t);
static const int MIN_BLOCK = (int)sizeof(block_t) + 16;

static block_t* free_list = NULL;
static int initialized = 0;

void exit(int code) {
    _syscall1(SYS_EXIT, (uint64_t)code);
    while (1);
}

void halt(void) {
    _syscall1(SYS_EXEC, (uint64_t)"shell");
    while (1);
}

void* sbrk(intptr_t increment) {
    return (void*)_syscall1(SYS_SBRK, (uint64_t)increment);
}

void* malloc(int size) {
    if (size <= 0) return NULL;
    if (!initialized) {
        initialized = 1;
        free_list = NULL;
    }

    int alloc_size = size + HEADER_SIZE;
    if (alloc_size < MIN_BLOCK) alloc_size = MIN_BLOCK;
    alloc_size = (alloc_size + HEAP_ALIGN - 1) & ~(HEAP_ALIGN - 1);

    block_t* prev = NULL;
    block_t* curr = free_list;

    while (curr) {
        if (curr->magic != MAGIC_FREE) {
            prev = curr;
            curr = curr->next;
            continue;
        }
        if (curr->size >= alloc_size) {
            if (curr->size >= alloc_size + MIN_BLOCK) {
                block_t* new_free = (block_t*)((char*)curr + alloc_size);
                new_free->size = curr->size - alloc_size;
                new_free->magic = MAGIC_FREE;
                new_free->next = curr->next;
                curr->size = alloc_size;
                if (prev) prev->next = new_free;
                else free_list = new_free;
            } else {
                if (prev) prev->next = curr->next;
                else free_list = curr->next;
            }
            curr->magic = MAGIC_USED;
            return (void*)((char*)curr + HEADER_SIZE);
        }
        prev = curr;
        curr = curr->next;
    }

    int pages = (alloc_size + 0xFFF) / 0x1000 + 1;
    int chunk_size = pages * 0x1000;

    block_t* new_block = (block_t*)sbrk(chunk_size);
    if ((intptr_t)new_block == -1) return NULL;

    new_block->size = chunk_size;
    new_block->magic = MAGIC_FREE;
    new_block->next = free_list;
    free_list = new_block;

    if (free_list->size >= alloc_size + MIN_BLOCK) {
        block_t* alloc_block = free_list;
        block_t* rem_block = (block_t*)((char*)alloc_block + alloc_size);
        rem_block->size = alloc_block->size - alloc_size;
        rem_block->magic = MAGIC_FREE;
        rem_block->next = alloc_block->next;
        alloc_block->size = alloc_size;
        alloc_block->magic = MAGIC_USED;
        free_list = rem_block;
        return (void*)((char*)alloc_block + HEADER_SIZE);
    }

    free_list = new_block->next;
    new_block->magic = MAGIC_USED;
    return (void*)((char*)new_block + HEADER_SIZE);
}

void free(void* ptr) {
    if (!ptr) return;
    block_t* blk = (block_t*)((char*)ptr - HEADER_SIZE);
    if (blk->magic != MAGIC_USED) return;

    blk->magic = MAGIC_FREE;
    blk->next = free_list;
    free_list = blk;
}
