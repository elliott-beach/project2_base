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
#include <stdbool.h>

int num_faults = 0;
int num_reads = 0;
int num_writes = 0;

struct disk* disk;
int nframes;

void evictPage(struct page_table *pt, int page);

void loadFrameIntoPage(struct page_table *pt, int page, int frame);

bool handleWriteFault(struct page_table *pt, int page);


int findFreeFrame(const struct page_table *pt);

/*
 * Handles a fault if it occured because the page needs write permissions.
 *
 * A page fault will occur when a page needs write permissions but does
 * not have them. In this case, we give the page write permissions but do not need to allocate
 * a page as we do for normal page faults.
 *
 * @return True if write fault was handled, else false.
 */
bool handleWriteFault(struct page_table *pt, int page) {
    bool readOnly = pt->page_bits[page] == PROT_READ;
    if(readOnly) {
        page_table_set_entry(pt, page, pt->page_mapping[page], PROT_READ|PROT_WRITE);
    }
    return readOnly;
}

/**
 * Find a frame that is not yet allocated in the page table.
 * @param pt The page table.
 * @return The frame number, or -1 if no free frame exists.
 */
int findFreeFrame(const struct page_table *pt) {
    for(int frame=0;frame<pt->nframes;frame++){
        bool success = true;
        for (int curr_page=0; curr_page < pt->npages; curr_page++){
            if(pt->page_mapping[curr_page] == frame && pt->page_bits[curr_page] != 0){
                success = false;
                break;
            }
        }
        if(success) return frame;
    }
    return -1;
}

void custom_handler(struct page_table *pt, int page ) {
    num_faults++;
    if(handleWriteFault(pt, page)) return;
    int frame = findFreeFrame(pt);

    // Find the page that is the greatest distance mod npages from the new page
    if(frame == -1){
        int page_out = 0;
        int dist;
        int max_dist = 0;
        for (int curr_page=0; curr_page < pt->npages; curr_page++){
            if(pt->page_bits[curr_page] != 0) {
                dist = (page - curr_page) % pt->npages;
                // if (dist < 0) dist *= -1; // I wanted to add this but it appears to break things  -Elliott
                if(dist > max_dist) {
                    max_dist = dist;
                    page_out = curr_page;
                }
            }
        }
//        int page_out = pt->npages-1;
        frame = pt->page_mapping[page_out];
        evictPage(pt, page_out);
    }

    loadFrameIntoPage(pt, page, frame);
}



void fifo_handler(struct page_table *pt, int page ) {
    num_faults++;
    static int curr_frame = 0;

    int page_out;
    int frame;
    bool evict = true;

    if(handleWriteFault(pt, page)) return;

    if(curr_frame < nframes) {
        frame = curr_frame;
        evict = false;
    } else {
        // If we evict a page from a frame, this is the frame
        frame = curr_frame % pt->nframes;
        for (int curr_page=0; curr_page < pt->npages; curr_page++){
            if(pt->page_mapping[curr_page] == frame && pt->page_bits[curr_page] != 0)
                // the page that maps to the frame is the one we want to evict
                page_out = curr_page;
        }
    }

    if(evict){
        evictPage(pt, page_out);
    }

    loadFrameIntoPage(pt, page, frame);
    curr_frame++;
}

void random_handler(struct page_table *pt, int page ) {
    num_faults++;
    if(handleWriteFault(pt, page)) return;
    int frame = findFreeFrame(pt);
    if(frame == -1){
        // Pick a random page that owns a frame.
        int curr_page;
        do {
            curr_page = rand() % pt->npages;
        } while(pt->page_bits[curr_page] == 0);

        frame = pt->page_mapping[curr_page];
        evictPage(pt, curr_page);
    }

    loadFrameIntoPage(pt, page, frame);
}

void loadFrameIntoPage(struct page_table *pt, int page, int frame) {
    // printf("page fault: setting page %d to frame %d\n", page, frame);
    num_reads++;
    disk_read(disk, page, page_table_get_physmem(pt) + frame * BLOCK_SIZE);
    page_table_set_entry(pt, page, frame, PROT_READ);
}

void evictPage(struct page_table *pt, int page) {
    // Only write to disk if the page has been modified
    if(pt->page_bits[page] == (PROT_READ|PROT_WRITE)) {
        int frame = pt->page_mapping[page];
        num_writes++;
        disk_write(disk, page, page_table_get_physmem(pt) + frame * BLOCK_SIZE);
    }
    page_table_set_entry(pt, page, 0, 0);
}

int main( int argc, char *argv[] ) {
    if(argc!=5) {
        printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
        return 1;
    }

    int npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    const char* algorithm = argv[3];
    const char *program = argv[4];

    disk = disk_open("myvirtualdisk",npages);
    if(!disk) {
        fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
        return 1;
    }


    page_fault_handler_t handler;
    if(!strcmp(algorithm, "rand")){
        handler = random_handler;
    } else if(!strcmp(algorithm, "fifo")){
        handler = fifo_handler;
    }else if(!strcmp(algorithm, "custom")){
        handler = custom_handler;
    }else {
        printf("Unknown paging algorithm!\n");
        exit(1);
    }

    struct page_table *pt = page_table_create( npages, nframes, handler );
    if(!pt) {
        fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
        return 1;
    }

    char *virtmem = page_table_get_virtmem(pt);

    if(!strcmp(program,"sort")) {
        sort_program(virtmem, npages * PAGE_SIZE);

    } else if(!strcmp(program,"scan")) {
        scan_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(program,"focus")) {
        focus_program(virtmem,npages*PAGE_SIZE);

    } else {
        fprintf(stderr,"unknown program: %s\n",argv[3]);
    }

    printf("%-5d%-5d%-5d\n", num_reads, num_writes, num_faults);
    page_table_delete(pt);
    disk_close(disk);

    return 0;
}
