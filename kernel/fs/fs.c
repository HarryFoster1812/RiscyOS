#include "io/sd_io_request_open.h"
#include <types.h>
#include <process.h>
#include <io/sd_io_request.h>
#include <io/io_request.h>
#include <fat_directory.h>
#include <fs_running_info.h>
#include <fat_entry.h>

#define SD_TX_RAM 20300

enum sd_io_request_type {
	ELF_LOAD,
	OPEN_FILE,
	READ_FILE,
	CLOSE_FILE,
	LIST_DIR,
};

void sd_irsq_handler();

extern void k_dbg_print(char* debugString);
extern void* kmalloc(int bytes);
extern void kfree(void* to_free);

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

enum file_open_states{
	LOAD_DIR, //this should look up all the dir within the sector if not in then go to the clustor inc
	PARSE_DIR, // this is callled when the dir lookup succeeds
	LOAD_FAT_TABLE,
	INCREMENT_CLUSTER,
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

												 return;

											 }
	}
}

extern int sd_start_read(uint32_t lba); // int return is the status of sd_start_read
extern int fat_start_read(uint32_t lba);
extern int sd_tail_read(); 
extern pcb_t* get_pcb_from_id(uint8_t pid);
extern void unblock_process(pcb_t* pcb);
extern int lba_to_sector(int lba);

extern FS_RUN_INFO* fs_running_info;

extern void* memset(void*, char, int);
extern void* memcpy(void* src, void* dest, int count);
extern int memcmp(void*, void*, int);

static inline int toupper(int chr)
{
	return ((chr >= 'a' && chr <= 'z') ? (chr - 32) : (chr));
}

int fat_valid_char(char c){

	if(c < 0x20)
		return 0;

	switch(c){
		case '"':
		case '*':
		case '+':
		case ',':
		case '/':
		case ':':
		case ';':
		case '<':
		case '=':
		case '>':
		case '?':
		case '[':
		case '\\':
		case ']':
		case '|':
			return 0;
	}

	return 1;
}

const char* get_next_path_component(const char* path, char out[13]) {
		const char* pathcpy = path;
    if(*pathcpy == '/')
        (pathcpy)++;

    if(*pathcpy == 0)
        return 0;

    int i = 0;

    while(*pathcpy && *pathcpy != '/') {
        out[i++] = *pathcpy;
        (pathcpy)++;
    }

    out[i] = 0;

    return pathcpy;
}

int make_fat_83_name(const char* input, char out[11]){

	memset(out, ' ', 11);

	char name[9];
	char ext[4];

	memset(name, 0, sizeof(name));
	memset(ext, 0, sizeof(ext));

	char* cpy = input;
	char* dot=NULL;
	while((*cpy++)){
		if(*cpy == '.'){
			dot = cpy;
			break;
		}
	}

	int name_len = 0;
	int ext_len = 0;

	// split name/ext
	if(dot){

		while(*input && input != dot){

			if(!fat_valid_char(*input))
				return 0;

			if(name_len < 8)
				name[name_len++] = toupper(*input);

			input++;
		}

		input = dot + 1;

		while(*input){

			if(!fat_valid_char(*input))
				return 0;

			if(ext_len < 3)
				ext[ext_len++] = toupper(*input);

			input++;
		}

	} else {

		while(*input){

			if(!fat_valid_char(*input))
				return 0;

			if(name_len < 8)
				name[name_len++] = toupper(*input);

			input++;
		}
	}

	memcpy(name, out, name_len);
	memcpy(ext, out + 8, ext_len);

	return 1;
}

void handle_open_file(sd_io_request* io_rq){
	while(1){
	switch(io_rq->state_machine){
		case LOAD_DIR: { // this assumes the lba of the dir is lba within the state
										 // given the current dir (lba) load into memory
										 sd_start_read(io_rq->lba);
										 return;
									 }
		case PARSE_DIR:{
										 sd_tail_read();
										 // the sector is now in memory
#define entries_per_sector 512/(sizeof(FAT_DIRECTORY))
										 void* sector_contents = (void*)0x20300;
										 FAT_DIRECTORY* dir = (FAT_DIRECTORY*)sector_contents;
										 
										 char component_str[13];
										 char component_83_format[11];
										 
										 char* component_ptr = ((sd_io_request_open*)io_rq)->ELF_FILE_NAME_PTR;
										 get_next_path_component(component_ptr, component_str);
										 make_fat_83_name(component_str, component_83_format);

										 for(int i = 0; i < entries_per_sector; i++){

											 FAT_DIRECTORY* entry = &dir[i];

											 // end of directory
											 if(entry->DIR_Name[0] == 0x00){
												 // target was not found
												 if(io_rq->request.proc_id){
													 // if it was not the kernel then we need to inform the requesting process
													 pcb_t* pcb = get_pcb_from_id(io_rq->request.proc_id);
													 unblock_process(pcb); // wake the process up
													 pcb->tf.TF_A0 = NULL;
												 }

												sd_request_queue = (sd_io_request*)io_rq->request.next;
												 kfree(io_rq);// free the request
												 return;
											 }

											 // deleted entry
											 if(entry->DIR_Name[0] == NOT_USED){
												 continue;
											 }

											 // there is a special case where the first character is a special character which indicates that it is meant to be the same as the not used marker
											 if (entry->DIR_Name[0] == USED_NAME0_E5)
												 entry->DIR_Name[0] = NOT_USED;

											 // long filename entry
											 if((entry->DIR_Attr & 0x0F) == 0x0F){
												 continue;
											 }

											 if(memcmp(entry->DIR_Name, component_str, 11) == 0){
												// FOUND THE COMPONENT
												int cluster_num = (entry->DIR_FstClusHI)<<sizeof(short) | entry->DIR_FstClusLO;
												int cluster_lba = cluster_to_lba(cluster_num);
												sd_io_request_open* io_open_req = ((sd_io_request_open*)io_rq);
												if(!(component_ptr = get_next_path_component(component_ptr, component_str))){
													// THIS IS THE LAST COMPONENT SO WE CAN JUST FILL OUT THE FILE STRUCT AND BE ON OUR WAY
													io_open_req->out_file->current_byte = 0;
													io_open_req->out_file->current_cluster = cluster_num;
													io_open_req->out_file->current_sector = 0;
													io_open_req->out_file->fd = 0;
													return;
												}
												// we are in a directory component
												if(io_open_req->PARENT_DIR_CLUSTER_STORAGE){
													*io_open_req->PARENT_DIR_CLUSTER_STORAGE = cluster_num;
												}
												io_rq->lba = cluster_lba;
												io_rq->state_machine=LOAD_DIR;
												sd_irsq_handler();
												return;
											 }

										 }
										 // increment the sector count
										 io_rq->lba++;
										 int curr_sector = lba_to_sector(io_rq->lba);
										 if (curr_sector >= fs_running_info->SECTORS_PER_CLUSTER){
											 io_rq->state_machine=LOAD_FAT_TABLE;
											 sd_irsq_handler();
											 return;
										 }
									 }
		case LOAD_FAT_TABLE:{
													fat_start_read(lba_to_cluster(io_rq->lba));
													io_rq->state_machine = INCREMENT_CLUSTER;
													return;
												}

		case INCREMENT_CLUSTER:{
														 int new_cluster = get_next_cluster(lba_to_cluster(io_rq->lba));
														 io_rq->lba = cluster_to_lba(new_cluster);
														 io_rq->state_machine = LOAD_DIR;
													 }
	}
	}
}
// we know it is a OPEN_FILE
// reinterperate the pointer
// genral step is:
// - read current directory
// - parse current directory

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
