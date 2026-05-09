#define SFN_BODY_ALL_LOWER 0x08
#define SFN_EXT_ALL_LOWER 0x10

#define NOT_USED 0xE5
#define USED_NAME0_E5 0x05  //; i have no idea why this exists but when DIR_Name[0] =  0xE5 and it is real data then it will be set to 0x5. Why not just disallow 0xE5 from being a part of the name?

typedef struct{

char DIR_Name [11]; 
unsigned char DIR_Attr;
unsigned char DIR_NTRes;

unsigned char DIR_CrtTimeTenth;
unsigned short DIR_CrtTime;
unsigned short DIR_CrtDate;

unsigned short DIR_LstAccDate;

unsigned short DIR_FstClusHI;
unsigned short DIR_WrtTime;
unsigned short DIR_WrtDate;

unsigned short DIR_FstClusLO;
unsigned int DIR_FileSize;
} FAT_DIRECTORY;

