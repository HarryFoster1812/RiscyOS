#pragma once
STRUCT
LDIR_Ord BYTE ; Sequence number
LDIR_Name1 BYTE 10 ; character 1-5 ??
LDIR_Attr BYTE ; This should be ATTR_LONG_FILE_NAME
LDIR_Type BYTE ; Must be 0
LDIR_Chksum BYTE
LDIR_Name2 BYTE 12 ; character 6 - 11
LDIR_FstClusLO BYTE 2  ; Must be zero to avoid any wrong repair by old disk utility
LDIR_Name3 BYTE 4 ; character 12 - 13
LDIR_STRUCT_SIZE ALIAS

; The maximum name length for LFN is 255, so that an LFN occupies upto 20 LFN entries
; The block of LFN+SFN are recorded on a contiguous entries. If any of these condition about LFN entry is not met, the LFN is invalid any longer

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

