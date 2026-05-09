#pragma once
#include <types.h>

extern int sd_start_read(uint32_t lba); // int return is the status of sd_start_read
extern int sd_tail_read(); 
extern int sd_tail_write(); 
extern int lba_to_sector(int lba);
extern int fat_calcualte_lba(int cluster_num);
extern int get_next_cluster(int cluster_num);
extern int cluster_to_lba(int cluster_num);
extern int lba_to_cluster(int lba);
