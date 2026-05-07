#include <types.h>
#include <io/sd_io_request.h>

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

enum sd_io_request_type {
	OPEN_FILE,
	READ_FILE,
	CLOSE_FILE,
	LIST_DIR,
	ELF_LOAD,
};

extern void k_dbg_print(char* debugString);

// When we want to find the next cluster given the current cluser then we need 
extern void fat_read(int cluster_num); // this will read the correct sector into memory
extern int get_next_cluster(int cluster_num);
extern int cluster_to_lba(int cluster_num);
extern int lba_to_cluster(int lba);

enum elf_load_states{
	
}; 

void handle_elf_load(sd_io_request* io_rq){
	switch(io_rq->state_machine){
	}
}

extern sd_io_request* sd_requeset_queue;
void sd_irsq_handler(){
	if(sd_requeset_queue == NULL) return; // if there is no request then we can not send it anywhere
	sd_io_request* request_to_service = sd_requeset_queue;
	
	switch((enum sd_io_request_type)request_to_service->request.type){
		case ELF_LOAD: return handle_elf_load(request_to_service);

	}

}

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
