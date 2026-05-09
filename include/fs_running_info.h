#pragma once
typedef struct {
	unsigned int FAT_BEGIN_LBA;
	int FAT_SECTOR_COUNT;
	int CLUSTER_BEGIN_LBA ;
	int ROOT_DIR_FIRST_CLUSTER ;
	unsigned char SECTORS_PER_CLUSTER ; 
}FS_RUN_INFO;
