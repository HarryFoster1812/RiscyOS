#ifndef FS_H
#define FS_H

void fs_init();
void fs_mount();
void fat_read_sector(int sector, char* buffer);
void fat_write_sector(int sector, const char* buffer);

typedef struct {
    int partition_start;

    int fat_start_lba;
    int data_start_lba;

    int sectors_per_cluster;
    int bytes_per_sector;

    int root_cluster;
} fat32_fs_t;





#endif
