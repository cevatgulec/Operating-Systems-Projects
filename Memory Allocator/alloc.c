#include <stdlib.h>
#include <unistd.h> 
#include <stddef.h> 
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>

#define ALIGNMENT sizeof(long)

#define BUFFER_SIZE 4096  
#define RETURN_THRESHOLD (1024 * 1024) 

static char *buffer = NULL;  
static size_t buffer_remaining = 0;  
static size_t total_allocated = 0; 
static size_t total_freed = 0;
static size_t num_brk_decrease_calls = 0;

char *find_highest_in_use_address();


typedef struct MemoryBlock {
    size_t size;
    struct MemoryBlock *next;
} MemoryBlock;

MemoryBlock *freeList = NULL;

size_t align(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}



void *kumalloc(size_t size) {
    size = align(size + sizeof(MemoryBlock));

    // try to find a free block from the free list
    MemoryBlock **current;
    MemoryBlock *block;  
    for (current = &freeList; *current; current = &(*current)->next) {
        if ((*current)->size >= size) {
            // if the remaining part of the block is large enough to be useful
            size_t remainingSize = (*current)->size - size;
            if (remainingSize > sizeof(MemoryBlock) + ALIGNMENT) {
                // create a new block in the remaining part
                MemoryBlock *newBlock = (MemoryBlock *)((char *)*current + size);
                newBlock->size = remainingSize;
                newBlock->next = (*current)->next;

                // update the size of the current block and remove it from the free list
                (*current)->size = size;
                MemoryBlock *allocatedBlock = *current;
                *current = newBlock;
                return ((char *)allocatedBlock) + sizeof(MemoryBlock);
            } else {
                // use the entire block
                MemoryBlock *allocatedBlock = *current;
                *current = allocatedBlock->next;
                return ((char *)allocatedBlock) + sizeof(MemoryBlock);
            }
        }
    }

    // check if there's enough space in the buffer
if (size > buffer_remaining) {
    if (size > BUFFER_SIZE) {
        // handle large allocations separately
        block = sbrk(size);
        if (block == (void *)-1) {
            return NULL;  
        }
        block->size = size;
        return ((char *)block) + sizeof(MemoryBlock);
    }

    
    buffer = sbrk(BUFFER_SIZE);
    if (

buffer == (void *)-1) {
return NULL; // sbrk failed
}
buffer_remaining = BUFFER_SIZE;
}

// Allocate from the buffer for normal allocations
block = (MemoryBlock *)buffer;
block->size = size;
buffer += size;
buffer_remaining -= size;

return ((char *)block) + sizeof(MemoryBlock);
}


void *kucalloc(size_t nmemb, size_t size)
{
    if (nmemb > 0 && size > 0 && (nmemb > SIZE_MAX / size)) {
        
        return NULL;
    }

    size_t total_size = nmemb * size;
    void *memory = kumalloc(total_size);
    
    if (memory != NULL) {
        //initialized allocated memory to zero
        memset(memory, 0, total_size);
    }

    return memory;
}

void kufree(void *ptr) {
    if (!ptr) return;

    MemoryBlock *blockToFree = (MemoryBlock *)((char *)ptr - sizeof(MemoryBlock));
    total_allocated -= blockToFree->size;
    total_freed += blockToFree->size;


    MemoryBlock *current;
    MemoryBlock *prev = NULL;

    for (current = freeList; current != NULL; prev = current, current = current->next) {
        if (blockToFree < current) {
            break;
        }
    }

    
    if (current != NULL && (char *)blockToFree + blockToFree->size == (char *)current) {
        blockToFree->size += current->size;
        blockToFree->next = current->next;
    } else {
        blockToFree->next = current;
    }

    
    if (prev != NULL && (char *)prev + prev->size == (char *)blockToFree) {
        prev->size += blockToFree->size;
        prev->next = blockToFree->next;
    } else if (prev == NULL) {
        freeList = blockToFree;
    } else {
        prev->next = blockToFree;
    }

    //checking if enough memory has been freed to return to the kernel
    if (total_freed >= RETURN_THRESHOLD && total_allocated < RETURN_THRESHOLD) {
        //perform logic to find the new break point
        char *highest_in_use = find_highest_in_use_address();

        void *new_brk = (void *) align((size_t) highest_in_use + ALIGNMENT);
        if (new_brk < sbrk(0)) {
            if (brk(new_brk) == -1) {
                
            } else {
                //increment the count only when brk successfully decreases the heap size
                num_brk_decrease_calls++;
                total_freed = 0;
            }
        }
    }
}


void *kurealloc(void *ptr, size_t size)
{
   
    if (!ptr) {
        return kumalloc(size);
    }

    // If size is 0, behave like free and return a unique pointer
    if (size == 0) {
        kufree(ptr);
        return kumalloc(1); // Allocate a minimal block to return a unique pointer
    }

    // Retrieve the size of the block that ptr points to
    MemoryBlock *block = (MemoryBlock *)((char *)ptr - sizeof(MemoryBlock));
    size_t oldSize = block->size - sizeof(MemoryBlock);

    // if the new size is less than or equal to the current size, return the original pointer
    if (size <= oldSize) {
        return ptr;
    }

    //allocate a new block with the new size
    void *newPtr = kumalloc(size);
    if (!newPtr) {
        return NULL; // Allocation failed, return NULL
    }

    //copying the contents from the old block to the new block
    memcpy(newPtr, ptr, oldSize);

    
    kufree(ptr);

    return newPtr;

}

char *find_highest_in_use_address() {
    char *highest = NULL;
    for (MemoryBlock *block = freeList; block != NULL; block = block->next) {
        char *block_end = ((char *) block) + block->size;
        if (highest == NULL || block_end > highest) {
            highest = block_end;
        }
    }
    return highest;
}

/*
 * Enable the code below to enable system allocator support for your allocator.
 * Doing so will make debugging much harder (e.g., using printf may result in
 * infinite loops).
 */
#if 1
void *malloc(size_t size) { return kumalloc(size); }
void *calloc(size_t nmemb, size_t size) { return kucalloc(nmemb, size); }
void *realloc(void *ptr, size_t size) { return kurealloc(ptr, size); }
void free(void *ptr) { kufree(ptr); }
#endif
