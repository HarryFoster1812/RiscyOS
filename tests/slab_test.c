#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../kernel/slab_allocator.c"

#define ASSERT_EQ(a,b) assert((a) == (b))
#define ASSERT_NEQ(a,b) assert((a) != (b))
// Simulated kernel memory
void* kmalloc(uint32_t size) {
     void* ptr=malloc(size);
     memset(ptr, 0, size);
     return ptr;
}

void kfree(void* ptr) {
    free(ptr);
}

void print_slab_header(slab_header_t* slab) {
    if (!slab) {
        printf("Slab: NULL\n");
        return;
    }
    printf("Slab at %p | object_size=%u | total_objects=%u | free_objects=%u | next=%p\n",
           (void*)slab, slab->object_size, slab->total_objects, slab->free_objects, (void*)slab->next);

    // Print free list addresses
    slab_node_t* node = slab->free_list;
    printf("  Free list:");
    while (node) {
        printf(" %p", (void*)node);
        node = node->next_free;
    }
    printf("\n");
}

void print_all_slabs(slab_header_t* head) {
    printf("=== Slab List ===\n");
    slab_header_t* slab = head;
    int i = 0;
    while (slab) {
        printf("Slab %d: ", i);
        print_slab_header(slab);
        slab = slab->next;
        i++;
    }
    printf("================\n");
}

void print_slab_objects(slab_header_t* slab) {
    if (!slab) return;

    uint32_t node_size = sizeof(slab_node_t*) + slab->object_size;
    uint8_t* start = (uint8_t*)slab + sizeof(slab_header_t);

    printf("Objects in slab %p:\n", (void*)slab);
    for (uint32_t i = 0; i < slab->total_objects; i++) {
        slab_node_t* node = (slab_node_t*)(start + i * node_size);
        int is_free = 0;

        // Check if node is in free list
        slab_node_t* temp = slab->free_list;
        while (temp) {
            if (temp == node) {
                is_free = 1;
                break;
            }
            temp = temp->next_free;
        }
        printf("  Object %u at %p -> %s\n", i, (void*)node->user_data, is_free ? "FREE" : "ALLOCATED");
    }
}

int count_slabs(slab_header_t* head){
  int counter=0;
  while(head){
    counter++;
    head = head->next;
  }
  return counter;
}

struct object_stats{
int free_objects;
int used_objects;
};

struct object_stats count_objects(slab_header_t* head){
  struct object_stats stats = {0,0};
  while(head){
    stats.free_objects = stats.free_objects+ head->free_objects;  
    stats.used_objects = stats.used_objects + (head->total_objects-head->free_objects);
    head = head->next;
  }
  return stats;
}


void test_init() {
    printf("[TEST] init\n");

    slab_header_t* head = NULL;
    slab_alloc_new(&head, 32, 4);

    ASSERT_NEQ(head, NULL);
    ASSERT_EQ(head->free_objects, 4);
    ASSERT_EQ(head->total_objects, 4);
}

void test_growth() {
    printf("[TEST] growth\n");

    slab_header_t* head = NULL;
    slab_alloc_new(&head, 16, 2);

    void* a = slab_get(&head);
    void* b = slab_get(&head);
    void* c = slab_get(&head); // new slab

    ASSERT_NEQ(a, NULL);
    ASSERT_NEQ(b, NULL);
    ASSERT_NEQ(c, NULL);

    ASSERT_EQ(count_slabs(head), 2);
}

void test_reuse() {
    printf("[TEST] reuse\n");

    slab_header_t* head = NULL;
    slab_alloc_new(&head, 32, 2);

    void* a = slab_get(&head);
    void* b = slab_get(&head);

    slab_free(&head, a);

    void* c = slab_get(&head);

    ASSERT_EQ(a, c);
}

void test_data_persistence() {
    printf("[TEST] data persistence\n");

    slab_header_t* head = NULL;
    slab_alloc_new(&head, sizeof(int), 2);

    int* a = (int*)slab_get(&head);
    *a = 1337;

    slab_free(&head, a);

    int* b = (int*)slab_get(&head);

    ASSERT_EQ(a, b);
    ASSERT_EQ(*b, 1337);
}

void test_full_free() {
    printf("[TEST] full free\n");

    slab_header_t* head = NULL;
    slab_alloc_new(&head, 16, 4);

    void* objs[8];

    for (int i = 0; i < 8; i++) {
        objs[i] = slab_get(&head);
    }

    for (int i = 0; i < 8; i++) {
        slab_free(&head, objs[i]);
    }

    ASSERT_EQ(count_objects(head).used_objects, 0);
}

void test_interleaved() {
    printf("[TEST] interleaved\n");

    slab_header_t* head = NULL;
    slab_alloc_new(&head, 24, 3);

    void* a = slab_get(&head);
    void* b = slab_get(&head);
    void* c = slab_get(&head);

    slab_free(&head, b);

    void* d = slab_get(&head);
    ASSERT_EQ(d, b);

    slab_free(&head, a);
    slab_free(&head, c);
    slab_free(&head, d);

    ASSERT_EQ(count_objects(head).used_objects, 0);
}

void test_stress() {
    printf("[TEST] stress\n");

    slab_header_t* head = NULL;
    slab_alloc_new(&head, 32, 8);

    void* ptrs[1000] = {0};

    srand(0);

    for (int i = 0; i < 10000; i++) {
        int idx = rand() % 1000;

        if (ptrs[idx]) {
            slab_free(&head, ptrs[idx]);
            ptrs[idx] = NULL;
        } else {
            ptrs[idx] = slab_get(&head);
            ASSERT_NEQ(ptrs[idx], NULL);
        }
    }

    for (int i = 0; i < 1000; i++) {
        if (ptrs[i]) slab_free(&head, ptrs[i]);
    }

    ASSERT_EQ(count_objects(head).used_objects, 0);
}

void test_single_object_slab() {
    printf("[TEST] single object slab\n");

    slab_header_t* head = NULL;
    slab_alloc_new(&head, 64, 1);

    void* a = slab_get(&head);
    void* b = slab_get(&head);

    ASSERT_EQ(count_slabs(head), 2);

    slab_free(&head, a);
    slab_free(&head, b);

    ASSERT_EQ(count_objects(head).used_objects, 0);
}

int main() {
    test_init();
    test_growth();
    test_reuse();
    test_data_persistence();
    test_full_free();
    test_interleaved();
    test_stress();
    test_single_object_slab();

    printf("\n ALL TESTS PASSED \n");
    return 0;
}

