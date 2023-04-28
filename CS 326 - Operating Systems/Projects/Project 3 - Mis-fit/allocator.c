/**
 * @file
 *
 * Explores memory management at the C runtime level.
 *
 * Author: @Omnitrix4m3
 *
 * To use (one specific command):
 * LD_PRELOAD=$(pwd)/allocator.so command
 * ('command' will run with your allocator)
 *
 * To use (all following commands):
 * export LD_PRELOAD=$(pwd)/allocator.so
 * (Everything after this point will use your custom allocator -- be careful!)
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "allocator.h"
#include "logger.h"

static struct mem_block *g_head = NULL; /*!< Start (head) of our linked list */

static unsigned long g_allocations = 0; /*!< Allocation counter */

pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER; /*< Mutex for protecting the linked list */

static size_t page_size = 4096;

static bool is_scribbling = false;

void *search(size_t region_size)
{
    puts("-- search() --");

    void *block = mmap(NULL, region_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(block == MAP_FAILED)
    {
        perror("mmap");

        return NULL;
    }

    return block;
}

void fill(struct mem_block *block, size_t requested_size, size_t block_size, struct mem_block *start)
{
    puts("-- FILL() --");

    block -> alloc_id = g_allocations++;

    sprintf(block -> name, "Allocation %lu", block -> alloc_id);

    block -> size = block_size;
    block -> usage = requested_size;
    block -> region_start = start;
    block -> next = NULL;
}

void *split(void *block, size_t size)
{
    puts("-- SPLIT() --");

    struct mem_block *current_block = block;
    struct mem_block *new_block = NULL;

    if(current_block -> usage == 0)
    {
        current_block -> usage = size;

        return current_block;
    }

    size_t new_block_size = current_block -> size;

    current_block -> size = current_block -> usage;

    new_block_size -= current_block -> usage;
    new_block = (void *)current_block + current_block -> size;

    fill(new_block, size, new_block_size, current_block -> region_start);

    if(current_block -> next != NULL)
    { 
        new_block -> next = current_block -> next;
    }

    current_block -> next = new_block;
    
    return new_block;
}

void *first_fit(size_t size)
{
    puts("-- FIRST_FIT() --");

    struct mem_block *current_block = g_head;
    
    while(current_block != NULL)
    {  
        if(current_block -> next == NULL)
        {
            return NULL;
        }
        
        if((current_block -> size - current_block -> usage) >= size)
        {            
            return current_block;
        }
    
        current_block = current_block -> next;
    }

    return NULL;
}

void *worst_fit(size_t size)
{
    puts("-- WORST_FIT() --");

    struct mem_block *current_block = g_head;
    struct mem_block *worst_block = NULL;

    size_t difference, check_difference;
    
    while(current_block != NULL)
    {
        check_difference = current_block -> size - current_block -> usage;
        
        if(check_difference >= size)
        {
            if(worst_block == NULL)
            {
                worst_block = current_block;
                difference = check_difference;
            }
            
            else
            {
                if(check_difference > difference)
                {
                    worst_block = current_block;
                    difference = check_difference;
                }
            }
        }

        current_block = current_block -> next;
    }

    return worst_block;
}

void *best_fit(size_t size)
{
    puts("-- BEST_FIT() --");

    struct mem_block *current_block = g_head;
    struct mem_block *best_block = NULL;

    size_t difference, check_difference;

    while(current_block != NULL)
    {
        check_difference = current_block -> size - current_block -> usage;

        if((current_block -> usage == 0 && current_block -> size == size) || (check_difference == size))
        {
            return current_block;
        }
        
        else if(check_difference > size)
        {
            if(best_block == NULL)
            {
                best_block = current_block;
                difference = check_difference;
            }

            if(check_difference != difference && check_difference < difference)
            {
                best_block = current_block;
                difference = check_difference;
            }
        }

        current_block = current_block -> next;
    }

    if(best_block == NULL)
    {
        return NULL;
    }

    return best_block;
}

void *reuse(size_t size)
{
    puts("-- REUSE() --");

    char *option = getenv("ALLOCATOR_ALGORITHM");

    void *ptr = NULL;

    if(option == NULL)
    {
        option = "first_fit";
    }
    
    if(strcmp(option, "first_fit") == 0)
    {
        ptr = first_fit(size);
    }
    
    else if(strcmp(option, "best_fit") == 0)
    {
        ptr = best_fit(size);
    }
    
    else if(strcmp(option, "worst_fit") == 0)
    {
        ptr = worst_fit(size);
    }
    
    else
    {
        return NULL;
    }

    if(ptr != NULL)
    {
        ptr = split(ptr, size);
    }

    return ptr;
}

void *malloc(size_t size)
{
    puts("-- MALLOC() --");

    pthread_mutex_lock(&alloc_mutex);

    if(size <= 0)
    {
        pthread_mutex_unlock(&alloc_mutex);
        return NULL;
    }

    size += sizeof(struct mem_block);

    if(size % 8 != 0)
    {
        size = size + (8 - size % 8);
    }

    size_t block_size = size;
    size_t num_pages = block_size / page_size;
    
    if((block_size % page_size) != 0)
    {
        num_pages += 1;
    }
    
    size_t region_size =  page_size * num_pages;

    struct mem_block *new_block = NULL;

    if(g_head == NULL)
    {
        char *scribble = getenv("ALLOCATOR_SCRIBBLE");

        if(scribble != NULL && atoi(scribble) == 1)
        {
            is_scribbling = true;
        }

        new_block = (struct mem_block *) search(region_size);
        
        if(new_block == NULL)
        {
            perror("search");

            pthread_mutex_unlock(&alloc_mutex);

            return NULL;
        }

        fill(new_block, size, region_size, new_block);

        new_block -> region_size = region_size;
        g_head = new_block;

        if(is_scribbling == true)
        {
            size_t scrib_size = new_block -> size - sizeof(struct mem_block);

            memset(new_block + 1, 0xAA, scrib_size); 
            is_scribbling = false;
        }
    }
    
    else
    { 
        char *scribble = getenv("ALLOCATOR_SCRIBBLE");

        if(scribble != NULL && atoi(scribble) == 1)
        {            
            is_scribbling = true;
        }

        new_block = (struct mem_block *) reuse(size);

        if(new_block == NULL)
        {
            new_block = (struct mem_block *) search(region_size);
            
            if(new_block == NULL)
            {
                perror("search");

                pthread_mutex_unlock(&alloc_mutex);

                return NULL;
            }

            fill(new_block, size, region_size, new_block);

            new_block -> region_size = region_size;

            struct mem_block *current_block = g_head;

            while(current_block -> next != NULL)
            {
                current_block = current_block -> next;
            }

            current_block -> next = new_block;
        }

        if(is_scribbling == true)
        {
            size_t scrib_size = new_block -> size - sizeof(struct mem_block);   
            memset(new_block + 1, 0xAA, scrib_size); 
            
            is_scribbling = false;
        }
    }

    pthread_mutex_unlock(&alloc_mutex);

    return new_block + 1;
}

void free(void *ptr)
{
    puts("-- FREE() --");

    pthread_mutex_lock(&alloc_mutex);

    if(ptr == NULL)
    {
        /* Freeing a NULL pointer does nothing */        
        pthread_mutex_unlock(&alloc_mutex);

        return;
    }

    struct mem_block *free_block = (struct mem_block *) ptr - 1;

    free_block -> usage = 0;

    bool region_empty = true;
    bool reset_head = false;

    struct mem_block *start = free_block -> region_start;
    struct mem_block *current_block = start;

    if(start == g_head)
    {
        reset_head = true;
    }

    while(current_block -> region_start == start)
    {
        if(current_block -> usage != 0)
        {
            region_empty = false;

            break;
        }
        
        else if(current_block -> next == NULL)
        {
            current_block = NULL;

            break;
        }
        
        else
        {
            current_block = current_block -> next;
        }
    }

    if(region_empty == true)
    {
        if(reset_head == true)
        {
            g_head = current_block;

            size_t empty_size = start -> region_size;
            int ret = munmap(start, empty_size);

            if(ret == -1)
            {
                perror("munmap");

                pthread_mutex_unlock(&alloc_mutex);

                return;
            }
        }
        
        else
        {
            struct mem_block *previous_block = g_head;
    
            while(previous_block -> next != start)
            {
                previous_block = previous_block -> next;
            }
            
            previous_block -> next = current_block;

            size_t empty_size = start -> region_size;

            int ret = munmap((void *)start, empty_size);

            if(ret == -1)
            {
                perror("munmap");
                pthread_mutex_unlock(&alloc_mutex);

                return;
            }
        }
    }

    pthread_mutex_unlock(&alloc_mutex);
}

void *calloc(size_t nmemb, size_t size)
{
    puts("-- CALLOC() --");

    void *ptr = malloc(nmemb * size);

    memset(ptr, 0x00, nmemb * size);
    
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    puts("-- REALLOC() --");

    if (ptr == NULL) {
        /* If the pointer is NULL, then we simply malloc a new block */
        pthread_mutex_unlock(&alloc_mutex);

        return malloc(size);
    }

    if (size == 0) {
        /* Realloc to 0 is often the same as freeing the memory block... But the
         * C standard doesn't require this. We will free the block and return
         * NULL here. */
        pthread_mutex_unlock(&alloc_mutex);
        
        free(ptr);
        
        return NULL;
    }

    pthread_mutex_lock(&alloc_mutex);

    size_t check_size = size + sizeof(struct mem_block);

    if(check_size % 8 != 0)
    {
        check_size = check_size + (8 - check_size % 8);
    }

    struct mem_block* current_block = (struct mem_block*) ptr - 1;

    if(current_block -> size >= check_size)
    {
        pthread_mutex_unlock(&alloc_mutex);

        current_block -> usage = check_size;

        return ptr;
    }

    pthread_mutex_unlock(&alloc_mutex);

    void *new_ptr = malloc(size);

    if(!new_ptr)
    {
        perror("malloc");

        return NULL;
    }
    
    memcpy(new_ptr, ptr, current_block -> usage);
    
    free(ptr); 

    return new_ptr;
}

/**
 * print_memory
 *
 * Prints out the current memory state, including both the regions and blocks.
 * Entries are printed in order, so there is an implied link from the topmost
 * entry to the next, and so on.
 */
void print_memory(void)
{
    puts("-- Current Memory State --");
    struct mem_block *current_block = g_head;
    struct mem_block *current_region = NULL;
    while (current_block != NULL) {
        if (current_block->region_start != current_region) {
            current_region = current_block->region_start;
            printf("[REGION] %p-%p %zu\n",
                    current_region,
                    (void *) current_region + current_region->region_size,
                    current_region->region_size);
        }
        printf("[BLOCK]  %p-%p (%lu) '%s' %zu %zu %zu\n",
                current_block,
                (void *) current_block + current_block->size,
                current_block->alloc_id,
                current_block->name,
                current_block->size,
                current_block->usage,
                current_block->usage == 0
                    ? 0 : current_block->usage - sizeof(struct mem_block));
        current_block = current_block->next;
    }
}

