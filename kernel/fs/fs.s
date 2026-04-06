fat_init:
	addi sp, sp, -8  
	sw ra, 4[sp]

	; read the bpb
	li a0, 0
	mv a1, sp
	call sd_start_read_single_block

fat_node_get:

cluster_to_sector:

next_cluster:

dir_lookup:

fat_read:

fat_write:

fat_mkdir:
