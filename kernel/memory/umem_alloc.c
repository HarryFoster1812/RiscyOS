#define USER_RAM_START 0x40000
#define ALLOC_ARRAY_BYTE_SIZE 512
// Each bit represents this many bytes:
#define MIN_BLOCK_SIZE ((256 * 1024) / (ALLOC_ARRAY_BYTE_SIZE * 8)) 

extern char alloc_bitmap[ALLOC_ARRAY_BYTE_SIZE];

/**
 * Sets count bits starting at start_bit to 1.
 */
void mark_bits_used(int start_bit, int count) {
	for (int i = 0; i < count; i++) {
		int bit = start_bit + i;
		alloc_bitmap[bit / 8] |= (1 << (bit % 8));
	}
}

/**
 * Sets count bits starting at start_bit to 0.
 */
void mark_bits_free(int start_bit, int count) {
	for (int i = 0; i < count; i++) {
		int bit = start_bit + i;
		alloc_bitmap[bit / 8] &= ~(1 << (bit % 8));
	}
}

void* ualloc_try_block(int num_bits) {
	int consecutive_zeros = 0;
	int start_bit = -1;

	// scan every bit in the bitmap
	for (int i = 0; i < ALLOC_ARRAY_BYTE_SIZE * 8; i++) {
		// check if bit i is free (0)
		if (!(alloc_bitmap[i / 8] & (1 << (i % 8)))) {
			if (consecutive_zeros == 0) start_bit = i;
			consecutive_zeros++;

			if (consecutive_zeros == num_bits) {
				// found a gap
				// mark it as used.
				mark_bits_used(start_bit, num_bits);

				// return base address + (offset * block_size)
				return (void*)(USER_RAM_START + (start_bit * MIN_BLOCK_SIZE));
			}
		} else {
			// reset counter if we hit a used bit
			consecutive_zeros = 0;
		}
	}

	return (void*)0;
}
