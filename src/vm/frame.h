#ifndef FRAME_H
#define FRAME_H

#include <stdio.h>
#include <stdlib.h>
#include "threads/thread.h"
#include "threads/palloc.h"

struct frame {
	struct list_elem elem;

	uint8_t *addr; // real frame address.
	void *vaddr;
	struct spage *spe;
	struct thread *thread;
	// struct spage_entry spage;
};

void frame_init ();
struct frame* frame_create();
void frame_free(struct frame *f);
struct frame* frame_allocate(struct spage *spe, enum palloc_flags stat);
void * frame_evict();

struct frame * find_frame_with_vaddr (void * vaddr);

void  evict_frame ();

void frame_free_with_addr(void *addr);

#endif