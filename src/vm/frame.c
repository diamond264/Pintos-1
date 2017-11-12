#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <list.h>

struct lock access_frame_table;
struct list frames;

void
init_frame_table ()
{
	list_init (&frames);
	lock_init (&access_frame_table);
}

struct list_elem *
get_frame_elem (void * frame)
{
	struct list_elem *iter;
	struct frame_entry *f;

	if (list_empty (&frames)) return NULL;

	for(iter = list_begin(&frames); iter != list_end(&frames); iter = list_next(iter))
	{
		f = list_entry(iter, struct frame_entry, elem);

		if(f->frame == frame)
			return iter;
	}

	return NULL;
}

void *
evict_frame ()
{
	lock_acquire (&access_frame_table);

	struct frame_entry *f = NULL;
	struct list_elem *iter;
	struct thread *t;
	struct spage_entry *spe;

	for(iter = list_begin(&frames); iter != list_end(&frames); iter = list_next(iter))
	{
		f = list_entry(iter, struct frame_entry, elem);
		t = f->thread;

		if (pagedir_is_accessed (t->pagedir, f->vaddr))
			pagedir_set_accessed (t->pagedir, f->vaddr, false);
		else
		{
			list_remove (iter);
			list_push_back (&frames, iter);
			break;
		}
	}
	//////
	t = f->thread;
	ASSERT(t);
	spe = spage_get_entry_from_thread (f->vaddr, t);

	if (spe == NULL)
	{
		spe = malloc (sizeof *spe);
		spe->vaddr = f->vaddr;
		hash_insert (&t->spage_table, &spe->elem);
		// insert 실패할경우?
	} else ASSERT(0);

	ASSERT(spe);

	size_t idx;

	if (pagedir_is_dirty (t->pagedir, spe->vaddr))
	{
		printf("ffff\n");
		
	}
	idx = swap_out (spe);
		// if (idx == BITMAP_ERROR)
		// {
		// 	ASSERT(0);
		// }

	memset (f->frame, 0, PGSIZE);
	spe->index = idx;
	pagedir_clear_page (t->pagedir, spe->vaddr);
	/////


	f->thread = thread_current ();
	f->vaddr = NULL;

	lock_release (&access_frame_table);

	return f->frame;
}

uint8_t *
allocate_frame (enum palloc_flags stat)
{
	uint8_t *frame;

	if (stat & PAL_USER)
    {
      if (stat & PAL_ZERO)
        frame = palloc_get_page (PAL_USER | PAL_ZERO);
      else
        frame = palloc_get_page (PAL_USER);
    }

	if (frame == NULL) 
	{
		frame = evict_frame ();
		if (frame == NULL)
			ASSERT(0);
	}
	else
	{
		insert_frame (frame);
	}
	return frame;
}

void
insert_frame (void * frame)
{
	struct frame_entry *f;

	lock_acquire (&access_frame_table);

	f = malloc (sizeof *f);
	// 말록 실패할 경우?
	f->frame = frame;
	f->thread = thread_current ();

	list_push_back (&frames, &f->elem);

	lock_release (&access_frame_table);
}

void
free_frame (void * frame)
{
	struct list_elem *e = get_frame_elem (frame);
	struct frame_entry *f;

	if (e == NULL) return;

	lock_acquire (&access_frame_table);
	
	f = list_entry(e, struct frame_entry, elem);
	list_remove (e);
	free (f);

	lock_release (&access_frame_table);
}




