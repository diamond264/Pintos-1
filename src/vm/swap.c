#include "vm/frame.h"
#include "vm/swap.h"
#include "vm/page.h"
#include "devices/disk.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SECTOR (DISK_SECTOR_SIZE / PGSIZE)

void
swap_init (void)
{
	swap_disk = disk_get (1,1);
	swap_bitmap = bitmap_create (disk_size (swap_disk));

	lock_init (&swap_lock);
}

void
swap_in (struct spage_entry *spe)
{
	lock_acquire (&swap_lock);

	size_t index = spe->index;
	size_t i;
	void *vaddr = spe->vaddr;

	for(i=0;i<PAGE_SECTOR;i++)
		disk_read (swap_disk, i+index, vaddr+i*DISK_SECTOR_SIZE);

	// bitmap_flip (swap_bitmap, index);
	bitmap_set_multiple(swap_bitmap, index, PAGE_SECTOR, 0);
	spe->index = -1;

	lock_release (&swap_lock);
}

size_t
swap_out (struct spage_entry *spe)
{
	lock_acquire (&swap_lock);

	size_t index = bitmap_scan_and_flip (swap_bitmap, 0, PAGE_SECTOR, 0);
	size_t page_sector = DISK_SECTOR_SIZE / PGSIZE * index;
	size_t i;
	void *vaddr = spe->vaddr;

	if (index == BITMAP_ERROR)
	{
		ASSERT(0);
		return NULL;
	}

	for(i=0;i<PAGE_SECTOR;i++)
		disk_write (swap_disk, i+index, vaddr+i*DISK_SECTOR_SIZE);

	lock_release (&swap_lock);

	return index;
}