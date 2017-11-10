#include "devices/disk.h"
#include <bitmap.h>

struct disk *swap_disk;
struct bitmap *swap_bitmap;
struct lock swap_lock;

void
swap_init ();

void
swap_in (struct spage_entry *spe);

bool
swap_out (struct spage_entry *spe);