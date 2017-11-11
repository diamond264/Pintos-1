#include <stdio.h>
#include <stdlib.h>
#include "threads/thread.h"

struct frame_entry {
	struct list_elem elem;

	tid_t tid;
	void *frame;
	void *vaddr;
	struct thread *thread;
	// struct spage_entry spage;
};

void
init_frame_table ();

struct frame *
find_frame_with_vaddr (void * vaddr);

void *
evict_frame ();

void
insert_frame (void * vaddr);

void
free_frame (void * vaddr);