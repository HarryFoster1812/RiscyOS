typedef enum{
	SD_STATE_IDLE ,
	SD_STATE_WAIT_READ ,
	SD_STATE_WAIT_WRITE ,
	SD_STATE_ERROR,
}SD_STATE_T;

typedef struct{
	char	SD_BLOCK_ADDRESSSING ;
	char SD_STATE             ;
} SD_INFO_T;
