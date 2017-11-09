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

struct list_elem *
get_frame_elem (void * vaddr)
{
	if (list_empty (&frames)) return NULL;

	struct list_elem *iter;
	struct frame_entry *frame_with_vaddr;

	for(iter = list_begin(&frames); iter != list_end(&frames); iter = list_next(iter))
	{
		frame_with_vaddr = list_entry(iter, struct frame_entry, elem);

		if(frame_with_vaddr->vaddr == vaddr)
			return iter;
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
	// LOCK 걸어 주어야 함
	struct list_elem *e = get_frame_elem (vaddr);
	struct frame_entry *f;

	if (e == NULL) return;
	
	f = list_entry(e, struct frame_entry, elem);
	list_remove (e);
	free (f);
}




