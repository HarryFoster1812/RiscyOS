#include <types.h>
#include <io/sd_io_request.h>
#include <io/io_request.h>

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
	ELF_LOAD,
	OPEN_FILE,
	READ_FILE,
	CLOSE_FILE,
	LIST_DIR,
};

void sd_irsq_handler();

extern void k_dbg_print(char* debugString);

// When we want to find the next cluster given the current cluser then we need 
extern void fat_read(int cluster_num); // this will read the correct sector into memory
extern int get_next_cluster(int cluster_num);
extern int cluster_to_lba(int cluster_num);
extern int lba_to_cluster(int lba);
extern void set_initial_dir(sd_io_request* req);
extern io_request* make_io_request(io_request_type type);
extern sd_io_request* sd_request_queue;

enum elf_load_states{
  ELF_OPEN_FILE,
  ELF_READ_HEADER,
  ELF_PARSE_HEADER,
  ELF_READ_CONTENT,
  ELF_FINISH,
  ELF_ERROR,
}; 

void fill_child_elf_request(sd_io_request* parent, io_request* child){
  child->next = (struct io_request*)parent;
  child->proc_id = parent->request.proc_id;
  // add to the head of the queue
  sd_request_queue = (sd_io_request*) child;

}


void handle_elf_load(sd_io_request* io_rq){
	switch(io_rq->state_machine){
    case ELF_OPEN_FILE:{
                         // create the open file request 
                        sd_io_request* elf_open_req = (sd_io_request*)make_io_request(SD_IO_REQUEST_OPEN);
                        if(elf_open_req == NULL){
                          //error happened - failed to make request
                          io_rq->state_machine = ELF_ERROR;
                          return handle_elf_load(io_rq);
                        }
                        fill_child_elf_request(io_rq, (io_request*)elf_open_req);
                        set_initial_dir(elf_open_req);
                        sd_irsq_handler();
                        return;

                       }
	}
}

void handle_open_file(sd_io_request* io_rq){
  switch(io_rq->state_machine){
  }
  // we know it is a OPEN_FILE
  // reinterperate the pointer
  // genral step is:
  // - read current directory
  // - parse current directory
  return;
}

void sd_irsq_handler(){
	if(sd_request_queue == NULL) return; // if there is no request then we can not send it anywhere
	sd_io_request* request_to_service = sd_request_queue;
	
	switch((io_request_type)request_to_service->request.type){
		case SD_IO_REQUEST_ELF: handle_elf_load(request_to_service); break;
		case SD_IO_REQUEST_OPEN: handle_open_file(request_to_service); break;
	}
  return;
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
