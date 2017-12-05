<<<<<<< HEAD
=======
#ifndef SWAP_H
#define SWAP_H

>>>>>>> origin/PJ-3-2
#include "devices/disk.h"
#include <bitmap.h>

struct disk *swap_disk;
struct bitmap *swap_bitmap;
struct lock swap_lock;

void
swap_init ();

void
<<<<<<< HEAD
swap_in (struct spage_entry *spe);

size_t
swap_out (struct spage_entry *spe);
=======
swap_in (struct spage *spe, void *addr);

size_t
swap_out (struct spage *spe, void *addr);

void swap_bitmap_free (struct spage *spe);

#endif
>>>>>>> origin/PJ-3-2
