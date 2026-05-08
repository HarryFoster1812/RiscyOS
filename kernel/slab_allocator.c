#include <types.h>

// This is what will repeat in memory after the header
typedef struct slab_node {
    struct slab_node* next_free;
    uint8_t user_data[]; 
} slab_node_t;

typedef struct slab_header {
	struct slab_header* next;   // next slab in the linked list
	slab_node_t* free_list;            // first free object in this slab
	uint32_t object_size;       // size of each object
	uint32_t total_objects;     // number of objects in this slab
	uint32_t free_objects;      // number of free objects
} slab_header_t;

// [ slab_header ][Pointer to Free][ object 0 ][Pointer to Free][ object 1 ] ... [Pointer to Free][ object n-1 ]


extern void* kmalloc(uint32_t size);
extern void kfree(void* object);

slab_header_t* slab_alloc_new(slab_header_t **head, uint32_t object_size, uint32_t num_objects) {
    // Each block needs size of pointer + size of user object
    uint32_t node_size = sizeof(slab_node_t*) + object_size;

    uint32_t slab_size = sizeof(slab_header_t) + (node_size * num_objects);
    uint8_t *mem = kmalloc(slab_size);
    if (!mem) return NULL;

    slab_header_t *slab = (slab_header_t*)mem;
    slab->object_size = object_size;
    slab->total_objects = num_objects;
    slab->free_objects = num_objects;

    slab->next = *head;
    *head = slab;

    // Grab the memory right after the header
    slab_node_t* current_node = (slab_node_t*)(mem + sizeof(slab_header_t));
    slab->free_list = (slab_node_t*)current_node; // Set head of free list

    // inialise the free list
    for (uint32_t i = 0; i < num_objects - 1; i++) {
        slab_node_t* next_node = (slab_node_t*)(((uint8_t*)current_node) + node_size);
        current_node->next_free = next_node;
        current_node = next_node;
    }
    current_node->next_free = NULL; // Last one points to NULL

    return slab;
}

void* slab_get(slab_header_t **head) {
  if(!head || !*(head)){return NULL;}

	slab_header_t *slab = *head;
	while (slab) {
		// check if there is a free object in the slab
		if (slab->free_objects > 0) {
			slab_node_t *node = slab->free_list;
			slab->free_list = node->next_free;
			slab->free_objects--;
			return (void*)node->user_data; 
		}
		slab = slab->next;
	}

	// no free object found, try to allocate new slab
	slab = slab_alloc_new(head, (*head)->object_size, (*head)->total_objects);
	if (!slab) return NULL; // allocation failed
	slab_node_t *node = slab->free_list;
	slab->free_list = node->next_free;
	slab->free_objects--;
	return (void*)node->user_data;
}

void slab_free(slab_header_t **head, void *obj) {
	slab_header_t *slab = *head;
	
	// walk slab linked list to find slab that the object belongs to
	while (slab) {
		uint8_t *start = (uint8_t *)slab + sizeof(slab_header_t);
		uint8_t *end = start + (sizeof(slab_node_t*) + slab->object_size) * slab->total_objects;

		if ((uint8_t *)obj >= start && (uint8_t *)obj < end) {
			// object belongs to this slab
			slab_node_t *node = (slab_node_t*)((uint8_t*)obj - sizeof(slab_node_t*));
			node->next_free = slab->free_list;
			slab->free_list = node;
			slab->free_objects++;

			// if 2 or more slabs fully free, free one
			int fully_free_count = 0;
			slab_header_t *tmp = *head;
			slab_header_t *prev = NULL;
			slab_header_t *free_target = NULL;
			slab_header_t *free_target_prev = NULL;

			while (tmp) {
				if (tmp->free_objects == tmp->total_objects){
					free_target = tmp;
					free_target_prev = prev;
					fully_free_count++;
				}
				prev = tmp;
				tmp = tmp->next;
			}
			if (fully_free_count >= 2) {
				// fix pointer
				free_target_prev->next = free_target->next;

				// free the slab 
				// underlying memory includes header + content
				kfree(free_target);
			}

			return;
		}
		slab = slab->next;
	}
}
