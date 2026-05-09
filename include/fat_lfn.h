#pragma once
#define ATTR_LONG_FILE_NAME 0x0F

#define LAST_LONG_ENTRY 0x40

typedef struct {
unsigned char LDIR_Ord;
char LDIR_Name1[10] ; //character 1-5 ??
unsigned char LDIR_Attr ; //This should be ATTR_LONG_FILE_NAME
char LDIR_Type ; //Must be 0
unsigned char LDIR_Chksum;
char LDIR_Name2 [12] ;// character 6 - 11
unsigned short  LDIR_FstClusLO ; //Must be zero to avoid any wrong repair by old disk utility
char LDIR_Name3 [4] ; //character 12 - 13
} LONG_FILE_NAME;

/*
 The LFN check sum algorithm:

uint8_t create_sum (const DIR* entry)
{
    int i;
    uint8_t sum;

    for (i = sum = 0; i < 11; i++) { // Calculate sum of DIR_Name[] field
        sum = (sum >> 1) + (sum << 7) + entry->DIR_Name[i];
    }
    return sum;
}

 */

