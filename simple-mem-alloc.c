/**
 * A very simple implementation of malloc 
 * (mem_alloc) and free (mem_free) for 
 * understanding purposes only. 
 * Actual implementations are much more 
 * complex than this, but I wanted to give 
 * people an idea of how the system allocates 
 * memory. Don't judge too harshly please!
 * @uthor: @ce
 * Bugs: While this code works as expected, several key elements are missing for it to look closer to a real malloc. Including:
 1 - The ability for continuous blocks to form a new block
 2 - The ability for an allocation to be spread across multiple blocks
 3 - Getting pages from the system (see below)
 4 - Other advanced features...
 * Version: 
 * See comment section for the changelog
 */

//These libraries are just used for showing things to the user and going over basic examples. Nothing more.
#include <cstdio>
#include <cstdlib>

#define uint_t unsigned int
#define HEADER 8

static uint_t MEM_POOL[128]; 
/* Note: This is a cheap way of getting memory from the system. It's a cheat basically. After all, memory has to come from somewhere. Typically, I'd use a system call like mmap, brk, and sbrk (RTFM) to request pages from the system but this isn't Linux and this is not an environment that I have as much freedom in so I am not focusing on where memory comes from. */
/* Note: The maximum size of our "MEM_POOL" is 128 bytes (128*sizeof(char)) If we ever try and allocate more than this, something horrible will happen, presumably in the form of a system crash. */

/*
 * This is the mechanism used to track what 
 * memory in our "pool" is used.
 */
struct free_pool
{
    void *ptr;
    size_t size;
};
typedef struct free_pool free_entry_t;

free_entry_t FREE_POOL[128] = 
{
    (free_entry_t){
        .ptr = MEM_POOL,
        .size = 128,
    },
};

static uint_t POOL_USED = 1;
free_entry_t *find_free_space(size_t);
void check_free_list();
void print_free_pool();
/**
 * You may wonder what exactly we mean by a 
 * void pointer. If a char ptr is a memory 
 * location at which one or more characters 
 * are stored, does that mean that a void 
 * pointer is a location where one or more 
 * voids are? Well no. Void in this case 
 * means unknown. So it's a memory address 
 * that contains an unknown value of an 
 * unknown type.
 * In any event, this function will attempt 
 * to return a pointer to some memory asked 
 * for by the user, and if it can't, it will 
 * return null.
 */
void *mem_alloc(size_t size)
{
    /* A note on size 0 pointers. Some mallocs handle this by returning a special pointer that can be directly passed to free(), others simply return null. This implementation does the latter. */
    if(size < 1) return NULL;
    //Add metadata header to size
    size += HEADER;
    //1st, is there memory available?
    free_entry_t *space;
    space = find_free_space(size);
    if(!space)
        return NULL;
    //add the overhead and send user what was requested
    void *base_ptr;
    size_t *size_ptr;
    void *user_ptr;

    base_ptr = space->ptr;
    size_ptr = (size_t*)space->ptr;
    user_ptr = space->ptr = space->ptr + (size_t)8;
    *size_ptr = size;
    //Update the pointer to the FREE_LIST
    space->ptr = (void*)((char*)space->ptr + size);
    space->size -= size;
    printf("----ALLOCATION----\n");
    print_free_pool();
    return user_ptr;
}

/**
 * Utility function that finds a free entry
 * for the free list. Basically, we iterate 
 * through the FREE_LIST and return the first 
 * entry that's big enough to hold the 
 * allocation
 */
free_entry_t *find_free_space(size_t size)
{
    //implementation of a best-fit algorithm
    free_entry_t *entry, *best_fit = FREE_POOL;
    for(uint_t i = 0; i < POOL_USED; i++)
    {
        entry = &FREE_POOL[i];
        if(entry->size >= size && best_fit->size > entry->size)
            best_fit = entry;
    }
    return best_fit->size >= size ? best_fit : NULL;
}
/**
 * The counterpart to mem_alloc, this 
 * function will free the memory requested by 
 * our mem_alloc function back to the 
 * MEM_POOL, if it can be identified.
 */
void mem_free(void *ptr)
{
    free_entry_t *entry = &FREE_POOL[POOL_USED];
    void *base_ptr = ptr - (size_t)8;
    size_t *size_ptr = (size_t*)base_ptr;
    size_t size = *size_ptr;
    
    //update the table
    entry->ptr = base_ptr;
    entry->size = size;
    ++POOL_USED;
    check_free_list();
    
    
    printf("----FREE----\n");
    print_free_pool();
}

void check_free_list()
{
    /* Iterate through the free list and remove zero sized entries */
    free_entry_t *entry;
    int i;
    int count = 0;
    for(i = 0; i < POOL_USED - 1; i++)
    {
        entry = &FREE_POOL[i];
        if(!entry->size)
        {
            FREE_POOL[i] = FREE_POOL[i + 1];
            ++count;
        }
    }
    POOL_USED -= count; 
}

//Driver used to show off our allocator
int main()
{
    print_free_pool();
    char *a, *b, *c, *d;
    //In keeping with good practices, check the return value here
    a = (char*)mem_alloc(4);
    if(!a)
    {
        perror("Fatal error: malloc");
        exit(EXIT_FAILURE);
    }
    printf("a->%p\n", a);
    b = (char*)mem_alloc(8);
    if(!b)
    {
        perror("Fatal error: malloc");
        exit(EXIT_FAILURE);
    }
    printf("b->%p\n", b);
    c = (char*)mem_alloc(16);
    if(!c)
    {
        perror("Fatal error: malloc");
        exit(EXIT_FAILURE);
    }
    printf("c->%p\n", c);
    /* As we are working more with memory, I don't care about actual values right now. We will be looking at the actual addresses here. */
    
    /* Now we will do an experiment. Theoretically, if I free one of these pointers, returning the memory to the system and request a new allocation of the same size, I should get the same value for addresses. */
    mem_free(b);
    d = (char*)mem_alloc(8);
    if(!d)
    {
        perror("Fatal error: malloc");
        exit(EXIT_FAILURE);
    }
    printf("d->%p\n", d);
    mem_free(a);
    mem_free(c);
    mem_free(d);
    /*//To test fail condition, uncomment this
    char *e = (char*)mem_alloc(128);
    if(!e)
    {
        perror("Fatal error: malloc");
        exit(EXIT_FAILURE);
    }
    */
    return 0;
}

/**
 * Utility function to print the contents of 
 * the FREE_POOL 
 */
void print_free_pool()
{
    printf("FREE POOL:\n");
    free_entry_t *entry;
    for(uint_t i = 0; i < POOL_USED; i++)
    {
        entry = &FREE_POOL[i];
        printf("\t%p (%u)\n", entry->ptr, entry->size);
    }
}
