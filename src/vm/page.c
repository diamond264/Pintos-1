<<<<<<< HEAD
#include "vm/frame.h"
#include "vm/swap.h"
#include "vm/page.h"
=======
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
>>>>>>> origin/PJ-3-2
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include <stdio.h>
<<<<<<< HEAD
#include <stdlib.h>

void
spage_load (struct spage_entry *spe)
{
	struct thread *curr = thread_current ();
	void *frame = allocate_frame (PAL_USER);
	// bool writable = spe->writable;

	if (frame == NULL) return;

	bool success = pagedir_set_page (curr->pagedir, spe->vaddr, frame, true);
	
	if (success)
	{
		swap_in (spe);
		//hash_delete (&curr->spage_table, &spe->elem);
	}
	else {
		ASSERT(0);
		free_frame (frame);
		palloc_free_page (frame);
	}
}

struct spage_entry *
spage_get_entry (void *vaddr)
{
	struct thread *curr = thread_current ();
	struct spage_entry spe;

	spe.vaddr = vaddr;
	struct hash_elem *spe_elem = hash_find (&curr->spage_table, &spe.elem);

	if (spe_elem == NULL)
		return NULL;

	return hash_entry (spe_elem, struct spage_entry, elem);
}

struct spage_entry *
spage_get_entry_from_thread (void *vaddr, struct thread *t)
{
	struct spage_entry spe;

	spe.vaddr = vaddr;
	struct hash_elem *spe_elem = hash_find (&t->spage_table, &spe.elem);

	if (spe_elem == NULL)
		return NULL;

	return hash_entry (spe_elem, struct spage_entry, elem);
}

struct spage_entry *
spage_insert_upage (uint8_t *upage, bool writable)
{
	struct spage_entry *spe;
	spe = malloc (sizeof *spe);
	if (spe == NULL) 
	{
		ASSERT(0);
		return NULL;
	}

	spe->vaddr = upage;
	spe->writable = writable;
	spe->index = -1;

	struct hash *h = &thread_current ()->spage_table;
	if (!hash_insert (h, &spe->elem))
	{
//		ASSERT(0);
		return NULL;
	}

	return spe;
}

struct hash_elem *
spage_insert_entry (struct spage_entry *spe)
{
	struct thread *curr = thread_current ();
	ASSERT(&curr->spage_table);

	return hash_insert (&curr->spage_table, &spe->elem);
}

bool
hash_comp_func (const struct hash_elem *x,
    const struct hash_elem *y, void *aux UNUSED)
{
 	return hash_entry (x, struct spage_entry, elem)->vaddr 
 	< hash_entry (y, struct spage_entry, elem)->vaddr;
}

unsigned
hash_calc_func (const struct hash_elem *e, void *aux UNUSED)
{
	struct spage_entry *spe = hash_entry (e, struct spage_entry, elem);
	return hash_int ((int) spe->vaddr);
}

void
hash_free_func (const struct hash_elem *e, void *aux UNUSED)
{
	struct spage_entry *spe = hash_entry (e, struct spage_entry, elem);
=======
#include <stdb.h>

extern struct lock page_lock;

void spage_load (struct spage *spe)
{
	struct thread *curr = thread_current();
	struct frame *f = frame_allocate(spe, PAL_USER);
	uint8_t *kpage = f->addr;
	if(kpage == NULL) ASSERT(0);

	swap_in(spe, kpage);
	if(pagedir_get_page(curr->pagedir, spe->vaddr)!=NULL || !pagedir_set_page(curr->pagedir, spe->vaddr, kpage, spe->writable)){
		ASSERT(0);
	}
}

struct spage* spage_create(void *vaddr, int status, bool writable)
{
	struct spage *spe = malloc(sizeof (struct spage));

	if(spe == NULL) return NULL;

	spe->status = status;
	spe->vaddr = pg_round_down (vaddr);
	spe->writable = writable;
	spe->index = 0;
	spe->offset = 0;
	spe->is_zero = false;
	spe->fd = 0;
	spe->file = NULL;
	spe->is_over = false;
	spe->length_over = 0;

	struct thread *curr = thread_current();

	if(hash_insert(&curr->spage_table, &spe->elem) == NULL)
	{
		return spe;
	}
	else
	{
		free(spe);
		return NULL;
	}
}

int spage_free(struct spage* spe)
{
	hash_delete(&thread_current()->spage_table, &spe->elem);
	free(spe);
}

struct spage* find_spage (void *vaddr)
{
	struct thread *curr = thread_current ();
	struct spage spe;

	spe.vaddr = pg_round_down (vaddr);
	struct hash_elem *spe_elem = hash_find (&curr->spage_table, &spe.elem);

	if (spe_elem == NULL)
		return NULL;

	return hash_entry (spe_elem, struct spage, elem);
}

bool hash_comp_func (const struct hash_elem *x, const struct hash_elem *y, void *aux UNUSED)
{
 	return hash_entry (x, struct spage, elem)->vaddr < hash_entry (y, struct spage, elem)->vaddr;
}

unsigned hash_calc_func (const struct hash_elem *e, void *aux UNUSED)
{
	struct spage *spe = hash_entry (e, struct spage, elem);
	return hash_int ((int) spe->vaddr);
}

void hash_free_func (const struct hash_elem *e, void *aux UNUSED)
{
	struct spage *spe = hash_entry (e, struct spage, elem);
>>>>>>> origin/PJ-3-2

	if (spe == NULL) return;

	//조건문에 넣어야 하려나?
<<<<<<< HEAD
	bitmap_flip (swap_bitmap, spe->index);
	free (spe);
	return;
}

void
hash_table_init (struct hash *h)
{
	hash_init (h, hash_calc_func, hash_comp_func, NULL);
}

void
stack_growth (void *addr)
{
	void *frame = allocate_frame (PAL_USER | PAL_ZERO);
	struct thread *curr = thread_current ();
	bool success = false;

	if (frame)
	{
		success = pagedir_set_page (curr->pagedir, pg_round_down (addr), frame, true);
		if (!success)
		{
			free_frame (frame);
			palloc_free_page (frame);
		}
	}
}


=======
	if (spe->status == SWAP)
		swap_bitmap_free (spe);
	
	frame_free_with_spage (spe);
	free (spe);

	return;
}

void stack_growth (void *addr)
{
	lock_acquire(&page_lock);
	addr = pg_round_down (addr);
	struct spage *spe = spage_create (addr, PAGE, true);
	struct frame *f = frame_allocate (spe, PAL_USER | PAL_ZERO);
	ASSERT(f && spe);

	lock_release(&page_lock);

	if(pagedir_get_page(thread_current ()->pagedir, addr)!=NULL 
		|| !pagedir_set_page(thread_current ()->pagedir, addr, f->addr, spe->writable))
		ASSERT(0);
}
>>>>>>> origin/PJ-3-2
