; FAT Bios Paramater block

STRUCT
BS_jmpBoot BYTE 3
BS_OEMName BYTE 8
BPB_BytsPerSec BYTE 2
BPB_SecPerClus BYTE
BPB_RsvdSecCnt BYTE 2 ; this is 1 for FAT 12 and 16 and typically 32 for FAT32
BPB_NumFATs BYTE ; this should be 2 but >= 1 is technically valid
BPB_RootEntCnt BYTE 2 ; for FAT32 this should be 0
BPB_TotSec16 BYTE 2
BPB_Media BYTE ; this is legacy and unused
BPB_FATSz16 BYTE 2 ; on FAT 32 this should be 0
BPB_SecPerTrk BYTE 2
BPB_NumHeads BYTE 2 
BPB_HiddSec BYTE 4
BPB_TotSec32 BYTE 4 ; this must be non-zero
; At this point the BPB becomes FAT-version specific
; I am going to assume it is FAT 32 and only support that
BPB_FATSz32 BYTE 4 ; count of sectors occupied by ONE FAT

; Bits 0-3 -- Zero-based number of active FAT
BPB_ExtFlags BYTE 2 
