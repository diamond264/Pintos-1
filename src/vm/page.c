#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include <stdio.h>
#include <stdlib.h>

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

	spe->valid = true;
}

struct spage* spage_create(void *vaddr, int status, bool writable)
{
	struct spage *spe = malloc(sizeof (struct spage));

	if(spe == NULL) return NULL;

	spe->status = status;
	spe->vaddr = pg_round_down (vaddr);
	spe->writable = writable;
	spe->index = -1;
	spe->valid = true;

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

void spage_ready_lazy(struct spage* spe, struct file *file, size_t read_bytes, size_t zero_bytes)
{
	spe->file = file;
	spe->page_read_bytes = read_bytes;
	spe->page_zero_bytes = zero_bytes;
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

	if (spe == NULL) return;

	//조건문에 넣어야 하려나?
	if (!spe->valid)
		swap_bitmap_free (spe);
	
	frame_free_with_spage (spe);
	free (spe);

	return;
}

void stack_growth (void *addr)
{
	addr = pg_round_down (addr);
	struct spage *spe = spage_create (addr, PAGE, true);
	struct frame *f = frame_allocate (spe, PAL_USER | PAL_ZERO);
	ASSERT(f && spe);

	if(pagedir_get_page(thread_current ()->pagedir, addr)!=NULL 
		|| !pagedir_set_page(thread_current ()->pagedir, addr, f->addr, spe->writable))
		ASSERT(0);
}