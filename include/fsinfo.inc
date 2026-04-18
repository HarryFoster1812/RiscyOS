;  FSInfo Sector Structure

FSI_LEAD_SIGNATURE EQU 0x41615252
FSI_STRUCT_SIGNATURE EQU 0x61417272
FSI_TRAILING_SIGNATURE EQU 0xAA550000

STRUCT
FSI_LeadSig WORD ; 0x41615252 (signature used to validate that this is in fact an FSInfo sector)
FSI_Reserved1 BYTE 480
FSI_StrucSig WORD ; 0x61417272.
FSI_Free_Count WORD ; indicates the last known free cluster count on the volume
FSI_Nxt_Free WORD ; cluster number at which the driver should start looking for free clusters
FSI_Reserved2 BYTE 12 ; should always be 0
FSI_TrailSig WORD ; 0xAA550000
FSI_STRUCT_SIZE ALIAS
