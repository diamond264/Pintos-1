#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <list.h>

struct list frame_list;
struct lock frame_lock;
struct lock evict_lock;

void frame_init()
{
	list_init(&frame_list);
	lock_init(&frame_lock);
	lock_init(&evict_lock);
}

struct frame* frame_create()
{
	struct frame *f = malloc(sizeof (struct frame));
	ASSERT(f);
	f->addr = NULL;
	f->vaddr = NULL;
	f->spe = NULL;
	f->thread = thread_current ();

	return f;
}

void frame_free(struct frame *f)
{
	lock_acquire(&frame_lock);
	if(f->addr != NULL)
	{
		palloc_free_page(f->addr);
		list_remove(&f->elem);
	}
	free(f);
	lock_release(&frame_lock);
}

void frame_free_with_addr(void *addr)
{
	lock_acquire(&frame_lock);
	ASSERT(0);

	struct list_elem *iter;
	for(iter = list_begin(&frame_list);iter != list_end(&frame_list);iter = list_next(iter)){
	    if(list_entry(iter, struct frame, elem)->addr == addr){
	      list_remove(iter);
	      free(list_entry(iter, struct frame, elem));
	      palloc_free_page(addr);
	      break;
	    }
	}
	lock_release(&frame_lock);
}

void frame_free_with_spage(struct spage *spe)
{
	lock_acquire(&frame_lock);

	struct list_elem *iter;
	for(iter = list_begin(&frame_list);iter != list_end(&frame_list);iter = list_next(iter)){
	    if(list_entry(iter, struct frame, elem)->spe == spe){
	    	// palloc_free_page(list_entry(iter, struct frame, elem)->addr);
	     	list_remove(iter);
	     	free(list_entry(iter, struct frame, elem));
	     	break;
	    }
	}
	lock_release(&frame_lock);
}

struct frame* frame_allocate(struct spage *spe, enum palloc_flags stat)
{
	if(stat & PAL_USER)
	{
		lock_acquire(&frame_lock);
		struct frame *f = malloc(sizeof (struct frame));

		uint8_t *addr = palloc_get_page(stat);
		while(addr == NULL)
		{
			frame_evict ();
			addr = palloc_get_page(stat);
		}

		f->spe = spe;
		f->addr = addr;
		f->thread = thread_current ();

		list_push_back(&frame_list, &f->elem);

		lock_release(&frame_lock);

		return f;
	}
	else
	{
		ASSERT(0);
		return NULL;
	}
}

void * frame_evict()
{
	struct frame *f = NULL;
	struct thread *t = NULL;
	struct list_elem *iter;

	iter = list_begin(&frame_list);

	f = list_entry (iter, struct frame, elem);
	t = f->thread;

	lock_acquire(&t->page_lock);
	struct spage *spe = f->spe;

	pagedir_clear_page (t->pagedir, spe->vaddr);
	swap_out (spe, f->addr);

	lock_release(&t->page_lock);
	if(f->addr != NULL)
	{
		palloc_free_page(f->addr);
		list_remove(&f->elem);
	}
	free(f);
}