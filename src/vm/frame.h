#include <stdio.h>
#include <stdlib.h>
#include "threads/thread.h"


struct list frames;
struct lock access_frame_table;

struct frame_entry {
	struct list_elem elem;

	tid_t tid;
	void *frame_addr;
	void *user_vaddr;
	struct thread *thread;
	struct spage_entry spage;
};

void
init_frame_table ();

// void
// set_frame (struct frame * f, void * vaddr, void * paddr);

// bool
// has_free_entry ();

// struct frame *
// find_frame_with_paddr (void * paddr);

struct frame *
find_frame_with_vaddr (void * vaddr);

void *
allocate_frame ();

void
insert_frame (void * vaddr);

void
free_frame (void * vaddr);