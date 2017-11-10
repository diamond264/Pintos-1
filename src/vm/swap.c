#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include <stdio.h>
#include <stdlib.h>

void
swap_init ()
{
	swap_disk = disk_get_role (BLOCK_SWAP);

	size_t bitmap_size = disk_size (swap_disk) * DISK_SECTOR_SIZE / PGSIZE;
	swap_bitmap = bitmap_create (bitmap_size);
	bitmap_set_all (swap_bitmap, true);
}

void
swap_in (struct spage_entry *spe)
{
	size_t index = spte->index;
	size_t page_sector = DISK_SECTOR_SIZE / PGSIZE * index;
	size_t i;
	void *vaddr = spe->vaddr;

	for(i=0;i<page_sector/index;i++)
		disk_read (swap_disk, i+page_sector, vaddr+i*DISK_SECTOR_SIZE);

	bitmap_flip (swap_table, index);
}

void
swap_out (struct spage_entry *spe)
{
	size_t index = bitmap_scan_and_flip (swap_map, 0, 1, true);
	size_t page_sector = DISK_SECTOR_SIZE / PGSIZE * index;
	size_t i;
	void *vaddr = spe->vaddr;

	if (index == BITMAP_ERROR)
		return;

	for(i=0;i<page_sector/index;i++)
		disk_read (swap_disk, i+page_sector, vaddr+i*DISK_SECTOR_SIZE);

	spte->index = index;
}