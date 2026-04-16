#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../kernel/memory/kmem.c" 

#define HEAP_SIZE 4096
static uint8_t fake_heap[HEAP_SIZE];

uint8_t* kernel_heap_start = fake_heap;
uint8_t* kernel_heap_end   = fake_heap + HEAP_SIZE;

#define ASSERT(x) assert(x)

static void dump_heap_state(const char* msg) {
  printf("\n[%s]\n", msg);

  heap_header_t* cur = (heap_header_t*)kernel_heap_start;

  while ((uint8_t*)cur < kernel_heap_end) {
    printf("Block @ %p | size=%u | used=%d\n",
        (void*)cur,
        GET_SIZE(cur),
        IS_USED(cur));

    if (GET_SIZE(cur) == 0) break;

    cur = (heap_header_t*)((uint8_t*)(cur + 1) + GET_SIZE(cur));
  }
}

void test_heap_init() {
  printf("[TEST] heap init\n");

  kheap_init();

  heap_header_t* h = (heap_header_t*)kernel_heap_start;

  ASSERT(GET_SIZE(h) > 0);
  ASSERT(!IS_USED(h));
}

void test_basic_alloc() {
  printf("[TEST] basic alloc\n");

  kheap_init();

  void* a = kmalloc(16);
  ASSERT(a != NULL);

  memset(a, 0xAA, 16);
}

void test_reuse() {
  printf("[TEST] reuse\n");

  kheap_init();

  void* a = kmalloc(16);
  kfree(a);

  void* b = kmalloc(16);

  ASSERT(a == b);
}

void test_split() {
  printf("[TEST] split\n");

  kheap_init();

  void* a = kmalloc(16);
  void* b = kmalloc(16);

  ASSERT(a != NULL);
  ASSERT(b != NULL);
}

void test_coalesce() {
  printf("[TEST] coalesce\n");

  kheap_init();

  void* a = kmalloc(16);
  void* b = kmalloc(16);
  void* c = kmalloc(16);

  kfree(b);
  kfree(a);

  heap_header_t* h = (heap_header_t*)kernel_heap_start;

  ASSERT(!IS_USED(h));
}

void test_full_merge() {
  printf("[TEST] full merge\n");

  kheap_init();

  void* a = kmalloc(32);
  void* b = kmalloc(32);
  void* c = kmalloc(32);

  kfree(a);
  kfree(b);
  kfree(c);

  heap_header_t* h = (heap_header_t*)kernel_heap_start;

  ASSERT(!IS_USED(h));
}

void test_fragmentation() {
  printf("[TEST] fragmentation\n");

  kheap_init();

  void* ptrs[50];

  for (int i = 0; i < 50; i++) {
    ptrs[i] = kmalloc(8);
  }

  for (int i = 0; i < 50; i += 2) {
    kfree(ptrs[i]);
  }

  for (int i = 0; i < 25; i++) {
    void* p = kmalloc(8);
    ASSERT(p != NULL);
  }
}

void test_alternating() {
  printf("[TEST] alternating\n");

  kheap_init();

  void* a = kmalloc(8);
  void* b = kmalloc(8);
  void* c = kmalloc(8);

  kfree(b);
  void* d = kmalloc(8);

  ASSERT(d == b);

  kfree(a);
  kfree(c);
  kfree(d);
}

void test_boundary() {
  printf("[TEST] boundary\n");

  kheap_init();

  char* a = kmalloc(16);

  for (int i = 0; i < 64; i++) {
    a[i] = 0xAA;
  }

  kfree(a);

  heap_header_t* h = (heap_header_t*)kernel_heap_start;

  ASSERT(!IS_USED(h));
}

void test_stress() {
  printf("[TEST] stress\n");

  kheap_init();

  void* ptrs[200] = {0};

  for (int i = 0; i < 10000; i++) {
    int idx = rand() % 200;

    if (ptrs[idx]) {
      kfree(ptrs[idx]);
      ptrs[idx] = NULL;
    } else {
      ptrs[idx] = kmalloc(16);
      ASSERT(ptrs[idx] != NULL);
    }
  }

  for (int i = 0; i < 200; i++) {
    if (ptrs[i]) kfree(ptrs[i]);
  }
}

int main() {
  printf("Dummy Kernel Heap Located at %p\n", &fake_heap);
  test_heap_init();
  test_basic_alloc();
  test_reuse();
  test_split();
  test_coalesce();
  test_full_merge();
  test_fragmentation();
  test_alternating();
  test_boundary();
  test_stress();

  printf("\n ALL KERNEL HEAP TESTS PASSED \n");
  return 0;
}
