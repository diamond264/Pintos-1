#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <list.h>
#include "userprog/syscall.h"

<<<<<<< HEAD
struct lock access_frame_table;
struct list frames;
=======
struct list frame_list;
extern struct lock file_lock;
>>>>>>> origin/PJ-3-2

void frame_init()
{
<<<<<<< HEAD
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
=======
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
>>>>>>> origin/PJ-3-2
	}
}

<<<<<<< HEAD
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
	} //else ASSERT(0);

	ASSERT(spe);

	size_t idx;

	if (pagedir_is_dirty (t->pagedir, spe->vaddr))
	{
		//printf("ffff\n");
		
=======
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
>>>>>>> origin/PJ-3-2
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

<<<<<<< HEAD
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
=======
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
>>>>>>> origin/PJ-3-2

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