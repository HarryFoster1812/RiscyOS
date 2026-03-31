#include "fs.h"

fat32_fs_t fat_fs_info;

static int internal_counter = 0;

void fs_init() {
    fat_fs_info.partition_start = 2048;
    fat_fs_info.bytes_per_sector = 512;
    fat_fs_info.sectors_per_cluster = 8;

    internal_counter = 1;
}

void fs_mount() {
    fat_fs_info.fat_start_lba = fat_fs_info.partition_start + 32;
    fat_fs_info.data_start_lba = fat_fs_info.fat_start_lba + 1024;

    internal_counter += fat_fs_info.data_start_lba;
}

void fat_read_sector(int sector, char* buffer) {
    // force memory writes + loop
    for (int i = 0; i < 16; i++) {
        buffer[i] = (char)(sector + i);
    }

    internal_counter += sector;
}

void fat_write_sector(int sector, const char* buffer) {
    int sum = 0;

    for (int i = 0; i < 16; i++) {
        sum += buffer[i];
    }

    internal_counter += sum + sector;
}

void fs_test() {
    char buffer[16];
    fat_read_sector(5, buffer);
    fat_write_sector(5, buffer);
}
