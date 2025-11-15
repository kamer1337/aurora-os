/**
 * Aurora OS - Memory Management Implementation
 * 
 * Basic memory allocation and management
 */

#include "memory.h"

/* Memory heap boundaries */
#define HEAP_START 0x00100000  /* 1 MB */
#define HEAP_SIZE  0x00100000  /* 1 MB heap size */
#define HEAP_END   (HEAP_START + HEAP_SIZE)

/* Page frame bitmap for physical memory management */
#define MAX_FRAMES 1024
static uint32_t frame_bitmap[MAX_FRAMES / 32];

/* Memory block structure for heap allocation */
typedef struct mem_block {
    size_t size;
    int free;
    struct mem_block* next;
} mem_block_t;

/* Heap management */
static mem_block_t* heap_start = NULL;
static uint32_t heap_initialized = 0;

/**
 * Mark a frame as used
 */
static void set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    frame_bitmap[idx] |= (1 << off);
}

/**
 * Mark a frame as free
 */
static void clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    frame_bitmap[idx] &= ~(1 << off);
}

/**
 * Test if a frame is allocated
 */
static uint32_t test_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    return (frame_bitmap[idx] & (1 << off));
}

/**
 * Find first free frame
 */
static uint32_t first_free_frame(void) {
    for (uint32_t i = 0; i < MAX_FRAMES / 32; i++) {
        if (frame_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t to_test = 1 << j;
                if (!(frame_bitmap[i] & to_test)) {
                    return i * 32 + j;
                }
            }
        }
    }
    return (uint32_t)-1;
}

/**
 * Allocate a physical frame
 */
static void* alloc_frame(void) {
    uint32_t frame = first_free_frame();
    if (frame == (uint32_t)-1) {
        return NULL;
    }
    set_frame(frame * PAGE_SIZE);
    return (void*)(frame * PAGE_SIZE);
}

/**
 * Free a physical frame
 */
static void free_frame(void* frame_addr) {
    clear_frame((uint32_t)frame_addr);
}

/**
 * Initialize heap allocator
 */
static void heap_init(void) {
    heap_start = (mem_block_t*)HEAP_START;
    heap_start->size = HEAP_SIZE - sizeof(mem_block_t);
    heap_start->free = 1;
    heap_start->next = NULL;
    heap_initialized = 1;
}

/**
 * Initialize memory management subsystem
 */
void memory_init(void) {
    /* Initialize page frame allocator */
    for (uint32_t i = 0; i < MAX_FRAMES / 32; i++) {
        frame_bitmap[i] = 0;
    }
    
    /* Mark kernel memory as used (first 1 MB) */
    for (uint32_t i = 0; i < 0x100000; i += PAGE_SIZE) {
        set_frame(i);
    }
    
    /* Initialize heap allocator */
    heap_init();
}

/**
 * Kernel memory allocation
 */
void* kmalloc(size_t size) {
    if (!heap_initialized) {
        return NULL;
    }
    
    if (size == 0) {
        return NULL;
    }
    
    /* Align size to 8 bytes */
    size = (size + 7) & ~7;
    
    mem_block_t* current = heap_start;
    
    /* First-fit allocation */
    while (current) {
        if (current->free && current->size >= size) {
            /* Check if we should split the block */
            if (current->size >= size + sizeof(mem_block_t) + 8) {
                mem_block_t* new_block = (mem_block_t*)((uint8_t*)current + sizeof(mem_block_t) + size);
                new_block->size = current->size - size - sizeof(mem_block_t);
                new_block->free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->free = 0;
            return (void*)((uint8_t*)current + sizeof(mem_block_t));
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * Free kernel memory
 */
void kfree(void* ptr) {
    if (!ptr || !heap_initialized) {
        return;
    }
    
    mem_block_t* block = (mem_block_t*)((uint8_t*)ptr - sizeof(mem_block_t));
    block->free = 1;
    
    /* Coalesce with next block if it's free */
    if (block->next && block->next->free) {
        block->size += sizeof(mem_block_t) + block->next->size;
        block->next = block->next->next;
    }
    
    /* Coalesce with previous block if it's free */
    mem_block_t* current = heap_start;
    while (current && current->next != block) {
        current = current->next;
    }
    
    if (current && current->free) {
        current->size += sizeof(mem_block_t) + block->size;
        current->next = block->next;
    }
}

/**
 * Virtual memory allocation
 */
void* vm_alloc(size_t size, uint32_t flags) {
    (void)flags; /* Unused for now */
    
    /* Calculate number of pages needed */
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    /* Allocate physical frames */
    void* first_frame = NULL;
    for (uint32_t i = 0; i < pages; i++) {
        void* frame = alloc_frame();
        if (!frame) {
            /* Allocation failed, clean up */
            return NULL;
        }
        if (i == 0) {
            first_frame = frame;
        }
    }
    
    return first_frame;
}

/**
 * Free virtual memory
 */
void vm_free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    /* Free the frame */
    free_frame(ptr);
}
