#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../kernel/memory/umem_alloc.c"

char alloc_bitmap[512];

#define BITS (512 * 8)

static void reset_bitmap() {
    memset(alloc_bitmap, 0, 512);
}

void test_bitmap_empty() {
    printf("[TEST] bitmap empty\n");

    reset_bitmap();

    for (int i = 0; i < BITS; i++) {
        assert((alloc_bitmap[i / 8] & (1 << (i % 8))) == 0);
    }
}

void test_mark_free() {
    printf("[TEST] mark + free\n");

    reset_bitmap();

    mark_bits_used(10, 5);
    mark_bits_free(10, 5);

    for (int i = 0; i < 5; i++) {
        int bit = 10 + i;
        assert(!(alloc_bitmap[bit / 8] & (1 << (bit % 8))));
    }
}

void test_single_alloc() {
    printf("[TEST] single alloc\n");

    reset_bitmap();

    void* p = ualloc_try_block(4);
    assert(p != NULL);

    int start_bit = ((int)p - USER_RAM_START) / MIN_BLOCK_SIZE;

    for (int i = 0; i < 4; i++) {
        int bit = start_bit + i;
        assert(alloc_bitmap[bit / 8] & (1 << (bit % 8)));
    }
}

void test_exact_fit() {
    printf("[TEST] exact fit\n");

    reset_bitmap();

    void* p1 = ualloc_try_block(8);
    void* p2 = ualloc_try_block(8);

    assert(p1 != NULL);
    assert(p2 != NULL);
}

void test_reuse() {
    printf("[TEST] reuse\n");

    reset_bitmap();

    void* p = ualloc_try_block(10);
    assert(p != NULL);

    int start = ((unsigned int)p - USER_RAM_START) / MIN_BLOCK_SIZE;

    mark_bits_free(start, 10);

    void* p2 = ualloc_try_block(10);

    assert(p2 == p);
}

void test_fragmentation() {
    printf("[TEST] fragmentation\n");

    reset_bitmap();

    void* blocks[20];

    for (int i = 0; i < 20; i++) {
        blocks[i] = ualloc_try_block(2);
        assert(blocks[i] != NULL);
    }

    for (int i = 0; i < 20; i += 2) {
        int start = ((unsigned int)blocks[i] - USER_RAM_START) / MIN_BLOCK_SIZE;
        mark_bits_free(start, 2);
    }

    void* p = ualloc_try_block(2);
    assert(p != NULL);
}

void test_full_exhaustion() {
    printf("[TEST] exhaustion\n");

    reset_bitmap();

    int allocations = 0;

    while (1) {
        void* p = ualloc_try_block(1);
        if (!p) break;
        allocations++;
    }

    printf("Allocated blocks: %d\n", allocations);

    assert(allocations > 0);
}

void test_alignment() {
    printf("[TEST] alignment\n");

    reset_bitmap();

    void* p = ualloc_try_block(3);

    unsigned int addr = (unsigned int)p;

    assert((addr - USER_RAM_START) % MIN_BLOCK_SIZE == 0);
}

void test_overlap_safety() {
    printf("[TEST] overlap\n");

    reset_bitmap();

    void* a = ualloc_try_block(4);
    void* b = ualloc_try_block(4);

    unsigned int a_start = ((unsigned int)a - USER_RAM_START) / MIN_BLOCK_SIZE;
    unsigned int b_start = ((unsigned int)b - USER_RAM_START) / MIN_BLOCK_SIZE;

    assert(b_start >= a_start + 4 || a_start >= b_start + 4);
}

#include <stdlib.h>

void test_stress() {
    printf("[TEST] stress\n");

    reset_bitmap();

    void* ptrs[500] = {0};

    for (int i = 0; i < 10000; i++) {
        int idx = rand() % 500;

        if (ptrs[idx]) {
            int start = ((unsigned int)ptrs[idx] - USER_RAM_START) / MIN_BLOCK_SIZE;
            mark_bits_free(start, 1);
            ptrs[idx] = NULL;
        } else {
            ptrs[idx] = ualloc_try_block(1);
        }
    }

    for (int i = 0; i < 500; i++) {
        if (ptrs[i]) {
            int start = ((unsigned int)ptrs[i] - USER_RAM_START) / MIN_BLOCK_SIZE;
            mark_bits_free(start, 1);
        }
    }
}

int main() {
    test_bitmap_empty();
    test_mark_free();
    test_single_alloc();
    test_exact_fit();
    test_reuse();
    test_fragmentation();
    test_full_exhaustion();
    test_alignment();
    test_overlap_safety();
    test_stress();

    printf("\n ALL UALLOC TESTS PASSED \n");
    return 0;
}
