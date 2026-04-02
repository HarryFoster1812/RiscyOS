typedef int uint32_t;
typedef char uint8_t;

typedef struct heap_header {
    uint32_t size_and_flags;
} heap_header_t;

#define USED_FLAG 1
#define NULL 0

#define GET_SIZE(h) ((h)->size_and_flags & ~USED_FLAG)
#define IS_USED(h)  ((h)->size_and_flags & USED_FLAG)

#define SET_USED(h) ((h)->size_and_flags |= USED_FLAG)
#define SET_FREE(h) ((h)->size_and_flags &= ~USED_FLAG)
#define SET_SIZE(h, s) ((h)->size_and_flags = (s) | ((h)->size_and_flags & USED_FLAG))

#define ALIGN4(x) (((x) + 3) & ~3)

// this tells C the symbol IS the address.
extern uint8_t kernel_heap_start[];
extern uint8_t kernel_heap_end[];

void kheap_init(void) {
    // With arrays, the name itself evaluates to the address. 
    uint32_t start = (uint32_t)kernel_heap_start;
    uint32_t end   = (uint32_t)kernel_heap_end;

    heap_header_t* heap_start = (heap_header_t*)start;
    
    // Explicitly initialize flags and size
    heap_start->size_and_flags = (end - start - sizeof(heap_header_t));
    SET_FREE(heap_start); 
}

void* kmalloc(uint32_t size) {
    size = ALIGN4(size);

    heap_header_t* current = (heap_header_t*)kernel_heap_start;

    while ((uint32_t)current < kernel_heap_end) {
        uint32_t block_size = GET_SIZE(current);

        if (!IS_USED(current) && block_size >= size) {
            // Split if possible
            if (block_size >= size + sizeof(heap_header_t) + 4) {
                heap_header_t* next =
                    (heap_header_t*)((uint8_t*)(current + 1) + size);

                next->size_and_flags =
                    (block_size - size - sizeof(heap_header_t));

                SET_FREE(next);
                SET_SIZE(current, size);
            }

            SET_USED(current);
            return (void*)(current + 1);
        }

        current = (heap_header_t*)((uint8_t*)(current + 1) + block_size);
    }

    return NULL;
}

// NOTE: ptr must be the address of heap header+1
void kfree(void* ptr) {
    if (!ptr) return;

    // Mark block as free
    heap_header_t* block = (heap_header_t*)ptr - 1;
    SET_FREE(block);

    // Full heap traversal + coalescing
    heap_header_t* current = (heap_header_t*)kernel_heap_start;

    while ((uint32_t)current < kernel_heap_end) {
        heap_header_t* next =
            (heap_header_t*)((uint8_t*)(current + 1) + GET_SIZE(current));

        // Stop if next is out of bounds
        if ((uint32_t)next >= kernel_heap_end)
            break;

        // If BOTH are free → merge
        if (!IS_USED(current) && !IS_USED(next)) {
            uint32_t new_size =
                GET_SIZE(current) + sizeof(heap_header_t) + GET_SIZE(next);

            current->size_and_flags = new_size; // stays free

            // IMPORTANT: do NOT advance current
            // we want to keep merging repeatedly
            continue;
        }

        // Otherwise move forward
        current = next;
    }
}
