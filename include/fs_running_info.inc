// this is the inforamtion that is stored and used at run time
STRUCT
FAT_BEGIN_LBA WORD ; Partition_LBA_Begin + Number_of_Reserved_Sectors
FAT_SECTOR_COUNT WORD 
CLUSTER_BEGIN_LBA WORD ; Partition_LBA_Begin + Number_of_Reserved_Sectors + (Number_of_FATs * Sectors_Per_FAT);
ROOT_DIR_FIRST_CLUSTER WORD; BPB_RootClus;
SECTORS_PER_CLUSTER BYTE ; BPB_SecPerClus;
STRUCT_ALIGN
FS_RUN_INFO_STRUCT_SIZE ALIAS
