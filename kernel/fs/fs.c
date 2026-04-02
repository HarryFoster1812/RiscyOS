typedef struct {
	// FILESYSTEM INFO PROPER
	unsigned char sectorsPerCluster;
	unsigned int firstDataSector;
	unsigned int totalSectors;
	unsigned short reservedSectors;
} FSMetadata;

typedef struct {
	unsigned int currentSector;
	unsigned char sectorFlags;
	unsigned int rootDirectorySize;
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
