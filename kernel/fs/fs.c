typedef struct {
	// FILESYSTEM INFO PROPER
	unsigned int firstDataSector;
	unsigned int totalSectors;
	unsigned short reservedSectors;
	unsigned char sectorsPerCluster;
} FSMetadata;

typedef struct {
	unsigned int currentSector;
	unsigned int rootDirectorySize;
	unsigned char sectorFlags;
}FSCurrentInfo;


extern void k_dbg_print(char* debugString);


// TODO:
// Implement cluster/section read, write
// Implement init function for getting the system information
// mkdir
// create 
// list_directoy
// inodes
// file handles
// seek
// fopen
// fclose
