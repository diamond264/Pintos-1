#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <list.h>

struct lock access_frame_table;
struct lock evict_lock;
struct list frames;

void
init_frame_table ()
{
	list_init (&frames);
	lock_init (&access_frame_table);
	lock_init (&evict_lock);
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

/*void *
evict_frame ()
{
	lock_acquire (&evict_lock);

	struct frame_entry *f = NULL;
	struct list_elem *iter;
	struct thread *t = NULL;
	struct spage_entry *spe;
	bool found = false;	

	for(iter = list_begin(&frames); iter != list_end(&frames); iter = list_next(iter))
	{
		f = list_entry(iter, struct frame_entry, elem);
		t = f->thread;

		if (pagedir_is_accessed (t->pagedir, f->vaddr)) {
			pagedir_set_accessed (t->pagedir, f->vaddr, false);
		}
		else
		{
			list_remove (iter);
			list_push_back (&frames, iter);
			found = true;
			break;
		}
	}

	if(found)
	{
		t = f->thread;
	}

	//////
	//t = f->thread;
	ASSERT(!(t == NULL));
	spe = spage_get_entry_from_thread (f->vaddr, t);

	if (spe == NULL)
	{
		printf("thread %d %x\n", thread_current()->tid, f->vaddr);
		printf("spe null\n");
		spe = spage_insert_upage(f->vaddr, true);
		//spe = malloc (sizeof *spe);

		//spe->vaddr = f->vaddr;
		//hash_insert (&t->spage_table, &spe->elem);
		//spage_insert_entry(spe);
		// insert 실패할경우?
	} //else ASSERT(0);

	ASSERT(spe);

	size_t idx;

	if (pagedir_is_dirty (t->pagedir, spe->vaddr))
	{
		//printf("ffff\n");
		
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

	lock_release (&evict_lock);

	return f->frame;
}*/

void evict_frame()
{
	lock_acquire(&evict_lock);

	struct frame_entry *f = NULL;
	struct thread *t = NULL;
	struct list_elem *iter;

	bool found = false;
	int iterCount = 0;

	for(iter = list_begin(&frames); iter != list_end(&frames); iter = list_next(iter))
	{
		iterCount++;

		f = list_entry(iter, struct frame_entry, elem);
		t = f->thread;

		/*if(t->pagedir == NULL)
		{
			continue;
		}*/

		if (pagedir_is_accessed (t->pagedir, f->vaddr)) {
			pagedir_set_accessed (t->pagedir, f->vaddr, false);
		}
		else // LRU에 걸리면
		{
			struct spage_entry *spe = spage_get_entry(f->vaddr);
			ASSERT(!(spe == NULL));

			if(spe->writable)
			{
				found = true;
				swap_out(spe);

				list_remove (iter);
				//list_push_back (&frames, iter);
				palloc_free_page(f->frame);
				free(f);
				break;
			}
			else
			{
				continue;
			}
		}
	}

	if(!found)
	{
		ASSERT(0);
	}

	lock_release(&evict_lock);
}

uint8_t *
allocate_frame (enum palloc_flags stat)
{
	uint8_t *frame = NULL;

	if (stat & PAL_USER)
	{
		int flag = 0;
		if(stat & PAL_ZERO)
			flag = PAL_USER | PAL_ZERO;
		else
			flag = PAL_USER;

		frame = palloc_get_page(flag);
		while(frame == NULL)
		{
			evict_frame();
			frame = palloc_get_page(flag);
		}

		insert_frame (frame);
	}

	return frame;
}

void
insert_frame (void * frame)
{
	lock_acquire (&access_frame_table);

	struct frame_entry *f = malloc (sizeof *f);
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




