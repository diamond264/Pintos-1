#include <stdio.h>
#include <stdlib.h>
#include "threads/thread.h"
#include "lib/kernel/hash.h"

struct spage_entry {
	struct hash_elem elem;

	void *vaddr;
	size_t index;
	bool loaded;
	bool swapped;
	bool writable;
};

void
spage_load (struct spage_entry *spe);

struct spage_entry *
spage_get_entry (void *vaddr);

struct hash_elem *
spage_insert_entry (struct spage_entry *spe);

void
spage_insert_upage (uint8_t *upage);

bool
hash_comp_func (const struct hash_elem *x, 
	const struct hash_elem *y, void *aux UNUSED);

unsigned
hash_calc_func (const struct hash_elem *e, void *aux UNUSED);

void
hash_free_func (const struct hash_elem *e, void *aux UNUSED);