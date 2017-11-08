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

/////////////////////////////////////////////////////////////
/// https://gist.github.com/EmilHernvall/953968/0fef1b1f826a8c3d8cfb74b2915f17d2944ec1d0
typedef struct vector_ {
    void** data;
    int size;
    int count;
} vector;

void vector_init(vector*);
int vector_count(vector*);
void vector_add(vector*, void*);
void vector_set(vector*, int, void*);
void *vector_get(vector*, int);
void vector_delete(vector*, int);
void vector_free(vector*);

void vector_init(vector *v)
{
    v->data = NULL;
    v->size = 0;
    v->count = 0;
}

int vector_count(vector *v)
{
    return v->count;
}

void vector_add(vector *v, void *e)
{
    if (v->size == 0) {
        v->size = 10;
        v->data = malloc(sizeof(void*) * v->size);
        memset(v->data, '\0', sizeof(void) * v->size);
    }

    // condition to increase v->data:
    // last slot exhausted
    if (v->size == v->count) {
        v->size *= 2;
        v->data = realloc(v->data, sizeof(void*) * v->size);
    }

    v->data[v->count] = e;
    v->count++;
}

void vector_set(vector *v, int index, void *e)
{
    if (index >= v->count) {
        return;
    }

    v->data[index] = e;
}

void *vector_get(vector *v, int index)
{
    if (index >= v->count) {
        return;
    }

    return v->data[index];
}

void vector_delete(vector *v, int index)
{
    if (index >= v->count) {
        return;
    }

    v->data[index] = NULL;

    int i, j;
    void **newarr = (void**)malloc(sizeof(void*) * v->count * 2);
    for (i = 0, j = 0; i < v->count; i++) {
        if (v->data[i] != NULL) {
            newarr[j] = v->data[i];
            j++;
        }
    }

    free(v->data);

    v->data = newarr;
    v->count--;
}

void vector_free(vector *v)
{
    free(v->data);
}
/////////////////////////////////////////////////////////////


struct disk* disk;
vector pagesStack;

void evictPage(struct page_table *pt, int page);

void loadFrameIntoPage(struct page_table *pt, int page, int frame);

void fifo_handler(struct page_table *pt, int page ) {
    static int curr_frame = 0;

    int page_out;
    int frame;
    bool evict = true;

    if(curr_frame < 10) {
        frame = curr_frame;
        evict = false;
    }
    else {
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
	int frame = -1;
	for(int f=0;f<pt->nframes;f++){

		// Find a frame that is not already allocated to a page.
		bool success = true;
		for (int curr_page=0; curr_page < pt->npages; curr_page++){
			if(pt->page_mapping[curr_page] == f && pt->page_bits[curr_page] != 0){
                success = false;
				break;
			}
		} if(success){
			frame = f;
			break;
		}
	}
    if(frame == -1){
        // Pick a random page that owns a frame.
        int curr_page;

        do {
            curr_page = (int)vector_get(&pagesStack, vector_count(&pagesStack) - 1);
            vector_delete(&pagesStack, vector_count(&pagesStack) - 1);
        } while(pt->page_bits[curr_page] == 0);

        frame = pt->page_mapping[curr_page];
        evictPage(pt, curr_page);
    }

    vector_add(&pagesStack, (void*)page);
    loadFrameIntoPage(pt, page, frame);
}

void loadFrameIntoPage(struct page_table *pt, int page, int frame) {
    printf("page fault: setting page %d to frame %d\n", page, frame);
    disk_read(disk, page, page_table_get_physmem(pt) + frame * BLOCK_SIZE);
    page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
}

void evictPage(struct page_table *pt, int page) {
    printf("eviction!\n");
    int frame = pt->page_mapping[page];
    disk_write(disk, page, page_table_get_physmem(pt) + frame * BLOCK_SIZE);
    page_table_set_entry(pt, page, 0, 0);
}

int main( int argc, char *argv[] ) {
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
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

	char *physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem, npages * PAGE_SIZE);

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
