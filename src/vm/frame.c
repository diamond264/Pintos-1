#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <list.h>
#include "userprog/syscall.h"

struct list frame_list;
extern struct lock file_lock;

void frame_init()
{
	list_init(&frame_list);
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
	if(f->addr != NULL)
	{
		palloc_free_page(f->addr);
		list_remove(&f->elem);
	}
	free(f);
}

void frame_free_with_addr(void *addr)
{
	struct list_elem *iter;
	for(iter = list_begin(&frame_list);iter != list_end(&frame_list);iter = list_next(iter)){
	    if(list_entry(iter, struct frame, elem)->addr == addr){
	      list_remove(iter);
	      free(list_entry(iter, struct frame, elem));
	      palloc_free_page(addr);
	      break;
	    }
	}
}

void frame_free_with_spage(struct spage *spe)
{
	struct list_elem *iter;
	for(iter = list_begin(&frame_list);iter != list_end(&frame_list);iter = list_next(iter)){
		struct frame *f = list_entry(iter, struct frame, elem);
		if(f->spe == spe && f->thread == thread_current())
		{
			// palloc_free_page(list_entry(iter, struct frame, elem)->addr);
			list_remove(iter);
			free(f);
			break;
		}
	}
}

struct frame* find_frame(struct spage *spe)
{
	struct list_elem *iter;
	for(iter = list_begin(&frame_list);iter != list_end(&frame_list);iter = list_next(iter)){
		struct frame *f = list_entry(iter, struct frame, elem);
		if(f->spe == spe && f->thread == thread_current())
		{
			// palloc_free_page(list_entry(iter, struct frame, elem)->addr);
			return f;
		}
	}

	return NULL;
}

struct frame* frame_allocate(struct spage *spe, enum palloc_flags stat)
{
	if(stat & PAL_USER)
	{
		//lock_acquire(&frame_lock);
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

		//lock_release(&frame_lock);

		return f;
	}
	else
	{
		ASSERT(0);
		return NULL;
	}
}

void* frame_evict()
{
	struct frame *f = NULL;
	struct thread *t = NULL;
	struct list_elem *iter;
	struct thread *curr = thread_current();

	iter = list_begin(&frame_list);

	f = list_entry (iter, struct frame, elem);
	t = f->thread;

	//lock_acquire(&t->page_lock);
	struct spage *spe = f->spe;

	pagedir_clear_page (t->pagedir, spe->vaddr);

	if(spe->status == MM_FILE)
	{
		if(pagedir_is_dirty(t->pagedir, spe->vaddr))
		{
			struct file* file = spe->file;
			lock_acquire(&file_lock);
			int write_len = spe->is_over ? spe->length_over : PGSIZE;
			file_write_at(file, f->addr, write_len, spe->offset);
			lock_release(&file_lock);
			spe->status = SWAP_MM;
		}
	}
	//else if (spe->fd >= 2)
	else if(spe->status == PAGE)
	{
		swap_out (spe, f->addr);
		spe->status = SWAP;
	}
	else printf("ㅅㅂ?\n");

	//palloc_free_page(f->addr);
	frame_free(f);
}