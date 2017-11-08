#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <list.h>

void
init_frame_table ()
{
	list_init (&frames);
}

// void
// set_frame (struct frame * f, void * vaddr, void * paddr)
// {
// 	f->vaddr = vaddr;
// 	f->paddr = paddr;
// }

// bool
// has_free_entry ()
// {
// 	void *vaddr = palloc_get_page (PAL_USER | PAL_ZERO);

// 	if (vaddr != NULL) return true;
// 	else return false;
// }

// struct frame *
// find_frame_with_paddr (void * paddr) 
// {
//   	if (list_empty (&frames)) return NULL;

// 	struct list_elem *iter;
// 	struct frame *frame_with_paddr;

// 	for(iter = list_begin(&frames); iter != list_end(&frames); iter = list_next(iter))
// 	{
// 		frame_with_paddr = list_entry(iter, struct frame, elem);

// 		if(frame_with_paddr->paddr == paddr)
// 			return frame_with_paddr;
// 	}

// 	return NULL;
// }

struct frame *
find_frame_with_vaddr (void * vaddr)
{
  	if (list_empty (&frames)) return NULL;

	struct list_elem *iter;
	struct frame_entry *frame_with_vaddr;

	for(iter = list_begin(&frames); iter != list_end(&frames); iter = list_next(iter))
	{
		frame_with_vaddr = list_entry(iter, struct frame_entry, elem);

		if(frame_with_vaddr->vaddr == vaddr)
			return frame_with_vaddr;
	}

	return NULL;
}

void *
allocate_frame ()
{
	void * vaddr = palloc_get_page (PAL_USER | PAL_ZERO);

	if (vaddr) {
		insert_frame (vaddr);
		return vaddr;
	}
	// else {
	// 	void * evicted_frame = frame_eviction ();
	// 	return evicted_frame;
	// }

	return vaddr;
}

void
insert_frame (void * vaddr)
{
	struct frame_entry *f;

	f = malloc (sizeof *f);
	f->vaddr = vaddr;
	f->tid = thread_current ()->tid;

	list_push_back (&frames, &f->elem);
}

void
free_frame (void * vaddr)
{
	struct frame_entry *f = find_frame_with_vaddr (vaddr);

	if (f == NULL) return;
	else {
		list_remove (&f->elem);
		palloc_free_page (vaddr);
		free (f);
	}
}




