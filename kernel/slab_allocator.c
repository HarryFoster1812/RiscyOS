#include "../include/types.h"

typedef struct slab_header {
	struct slab_header *next;   // next slab in the linked list
	void *free_list;            // first free object in this slab
	uint32_t object_size;       // size of each object
	uint32_t total_objects;     // number of objects in this slab
	uint32_t free_objects;      // number of free objects
} slab_header_t;

// [ slab_header ][ object 0 ][ object 1 ] ... [ object n-1 ]

extern void* kmalloc(uint32_t size);
extern void kfree(void* object);

slab_header_t* slab_alloc_new(slab_header_t **head, uint32_t object_size, uint32_t num_objects) {
	uint32_t slab_size = sizeof(slab_header_t) + (object_size+1) * num_objects; // object size +1 because each start will have a pointer to the next free item in that slab
	uint8_t *mem = kmalloc(slab_size);
	if (!mem) return NULL;

	slab_header_t *slab = (slab_header_t*)mem;
	slab->object_size = object_size;
	slab->total_objects = num_objects;
	slab->free_objects = num_objects;

	slab->next = *head; // push to head of slab list
	*head = slab;

	// Initialise free list
	uint8_t *objects = mem + sizeof(slab_header_t);
	slab->free_list = objects;
	for (uint32_t i = 0; i < num_objects - 1; i++) {
		*(void **)(objects + i * object_size) = (void *)(objects + (i + 1) * object_size);
	}
	*(void **)(objects + (num_objects - 1) * object_size) = NULL;

	return slab;
}

void* slab_get(slab_header_t **head, uint32_t object_size, uint32_t num_objects) {
	slab_header_t *slab = *head;
	while (slab) {
		// check if there is a free object in the slab
		if (slab->free_objects > 0) {
			void *obj = slab->free_list;
			slab->free_list = *(void **)obj; // update free list
			slab->free_objects--;
			return (uint8_t*)obj++; // actual object is obj+1
		}
		slab = slab->next;
	}

	// no free object found, try to allocate new slab
	slab = slab_alloc_new(head, object_size, num_objects);
	if (!slab) return NULL; // allocation failed
	void *obj = slab->free_list;
	slab->free_list = *(void **)obj;
	slab->free_objects--;
	return (uint8_t*)obj++; // actual object is obj+1
}

void slab_free(slab_header_t **head, void *obj) {
	slab_header_t *slab = *head;
	slab_header_t *prev = NULL;
	
	// walk slab linked list to find slab that the object belongs to
	while (slab) {
		uint8_t *start = (uint8_t *)slab + sizeof(slab_header_t);
		uint8_t *end = start + (slab->object_size+1) * slab->total_objects;

		if ((uint8_t *)obj >= start && (uint8_t *)obj < end) {
			// object belongs to this slab
			*((uint8_t**)obj-1) = (uint8_t*)slab->free_list;
			slab->free_list = obj;
			slab->free_objects++;

			// if 2 or more slabs fully free, free one
			int fully_free_count = 0;
			slab_header_t *tmp = *head;
			while (tmp) {
				if (tmp->free_objects == tmp->total_objects) fully_free_count++;
				tmp = tmp->next;
			}
			if (fully_free_count >= 2) {
				// free the slab at head of list (could optimize selection)
				slab_header_t *to_free = *head;
				*head = to_free->next;
				// underlying memory includes header + content
				kfree(to_free);
			}

			return;
		}
		prev = slab;
		slab = slab->next;
	}
}
