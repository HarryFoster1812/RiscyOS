STRUCT
PARTITION_STATUS BYTE ; Note: 0x80 is active 0x0 is inactive and aything else is invalid
PARTITION_FIRST_SECTOR_ADDRESS BYTE 3 ; the first absolute sector in partition
PARTITION_TYPE BYTE ;
PARTITION_LAST_SECTOR_ADDRESS BYTE 3 ; the first absolute sector in partition
PARTITION_LBA_FIRST WORD
PARTITION_SECTOR_COUNT WORD
