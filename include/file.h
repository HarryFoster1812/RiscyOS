#pragma once
typedef struct {
  unsigned int fd;
  unsigned int first_cluster;      // NEVER changes after open — the anchor for backward seek
  unsigned int current_cluster;
  unsigned int current_sector;
  unsigned int current_byte;
	unsigned int file_offset;        // absolute byte offset from start of file
	unsigned int file_size;          // needed for bounds checking and SEEK_END
	unsigned char owner_pid;
}FILE;
