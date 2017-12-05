#ifndef FRAME_H
#define FRAME_H

#include <stdio.h>
#include <stdlib.h>
#include "threads/thread.h"
#include "threads/palloc.h"

<<<<<<< HEAD
struct frame_entry {
	struct list_elem elem;

	tid_t tid;
	void *frame;
	void *vaddr;
=======
struct frame {
	struct list_elem elem;

	uint8_t *addr; // real frame address.
	void *vaddr;
	struct spage *spe;
>>>>>>> origin/PJ-3-2
	struct thread *thread;
	// struct spage_entry spage;
};

<<<<<<< HEAD
void
init_frame_table ();

struct frame *
find_frame_with_vaddr (void * vaddr);

void *
evict_frame ();

uint8_t *
allocate_frame (enum palloc_flags stat);

void
insert_frame (void * vaddr);

void
free_frame (void * vaddr);
=======
void frame_init ();
struct frame* frame_create();
void frame_free(struct frame *f);
struct frame* frame_allocate(struct spage *spe, enum palloc_flags stat);
void * frame_evict();

struct frame * find_frame_with_vaddr (void * vaddr);

void  evict_frame ();

void frame_free_with_addr(void *addr);
void find_frame_with_spage(struct spage *spe);

#endif
>>>>>>> origin/PJ-3-2
