SFN_BODY_ALL_LOWER EQU 0x08
SFN_EXT_ALL_LOWER EQU 0x10

NOT_USED EQU 0xE5
USED_NAME0_E5 EQU 0x05 ; i have no idea why this exists but when DIR_Name[0] =  0xE5 and it is real data then it will be set to 0x5. Why not just disallow 0xE5 from being a part of the name?
; Allowable characters for names:
; 0-9
; A-Z
; '!', '#', '$', '%', '&'
; '\'', '(', ')', '-', '@'
; '^', '_', '`', '{', '}', '~'

; NOTE: lower case characters are not allowed
; '.' direcorys SFN look like: ".       "
; '..' directory FSN look like "..      "


STRUCT
DIR_Name BYTE 11 ; Short file name in 8.3 (8 byte body + 3 byte extension)
DIR_Attr BYTE 
DIR_NTRes BYTE

; Directory creation Timestamp
DIR_CrtTimeTenth BYTE
DIR_CrtTime BYTE 2
DIR_CrtDate BYTE 2

DIR_LstAccDate BYTE 2 ; set to 0 if not availble

DIR_FstClusHI BYTE 2 ; Upper part of cluster number
; Directory Write Timestamp
DIR_WrtTime BYTE 2
DIR_WrtDate BYTE 2

DIR_FstClusLO BYTE 2 ; Lower part of cluster number. When the file size is zero, no cluster is assigned and this item must be zero
DIR_FileSize WORD
FAT_DIR_STRUCT_SIZE ALIAS


; NOTE: to delete a file just set 0xE5 to NAME[0], if it has a chain then the same need to be done for the entire chain
; to delete a directory, same as deleting a file apart from you do it for each file
