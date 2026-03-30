#ifndef FS_H
#define FS_H

void fs_init();
void fs_mount();
void fat_read_sector(int sector, char* buffer);
void fat_write_sector(int sector, const char* buffer);

#endif
