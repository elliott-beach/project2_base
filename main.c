/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct disk *disk;

// My understanding is that, when a page fault occurs, we need to evict data from a page.
// To do that, we must preserve memory writes by writing the frame where the fault occurs
// to the disk. Then, the next time the frame is loaded, the data will be preserved.
int count = 0;
void page_fault_handler( struct page_table *pt, int page ) {
	count++;
	//if(count == 25) exit(0);
	int frame = -1;
	for(int f=0; f < pt->nframes; f++){

		// Find a frame that is not already allocated to a page.
		int curr_page;
		for (curr_page=0; curr_page<pt->npages; curr_page++){
			if(pt->page_mapping[curr_page] == f){ /* && page_bits[page] != 0 */ // We could also check the page bits, but it doesn't matter too much.
				break;
			}
		} if(curr_page == pt->npages){
			frame = f;
			break;
		}

		// Else, evict a page.
		if(1 + f == pt->nframes){
			printf("eviction!\n");

			// Find first page that is in-memory
			for(curr_page=0;pt->page_bits[curr_page] == 0;curr_page++)
				;
			frame = pt->page_mapping[curr_page];

			// Write it to disk
			disk_write(disk, curr_page, page_table_get_physmem(pt) + frame * BLOCK_SIZE);

			// Mark it as out
			page_table_set_entry(pt, curr_page, 0, 0);

			break;
		}
	}
	disk_read(disk, page, page_table_get_physmem(pt) + frame * BLOCK_SIZE);
	printf("page fault: setting page %d to frame %d\n", page, frame);
	page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
}

int main( int argc, char *argv[] ) {
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	const char *program = argv[4];

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem, npages * 369);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);
	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
