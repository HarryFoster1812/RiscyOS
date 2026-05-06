; FAT Bios Paramater block
FAT_BPB_SIGNATURE EQU 0xaa55

; 0x0 - 0
; 0x4 - 4
; 0x8 - 8
; 0xC - 12
; 0x10- 16

STRUCT
; 0x0 - 0x2 
BS_jmpBoot BYTE 3

; 0x3 - 0xA (INITAL BYTE NOT ALIGNED)
BS_OEMName BYTE 8

; 0xB - 0xC (LOAD HALF)
BPB_BytsPerSec BYTE 2

; 0xD - 0xD (LOAD BYTE)
BPB_SecPerClus BYTE

; 0xE - 0xF (LOAD HALF?)
BPB_RsvdSecCnt BYTE 2 ; this is 1 for FAT 12 and 16 and typically 32 for FAT32

; 0x10 - 0x12 (LOAD HALF)
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
BPB_FSVer BYTE 2 ; FAT version
BPB_RootClus BYTE 4 ; First cluster number of the root directory
BPB_FSInfo BYTE 2 ; Sector of FSInfo structure in offset from top of the FAT32 volume
BPB_BkBootSec BYTE 2 ; backup boot sector in offset from top of the FAT32 volume
BPB_Reserved BYTE 12
BS_DrvNum BYTE
BS_Reserved BYTE
BS_BootSig BYTE
BS_VolID WORD
BS_VolLab BYTE 11
BS_FilSysType BYTE 8
BS_BootCode32 BYTE 420
BS_Sign BYTE 2 
