#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "devices/block.h"
#include <bitmap.h>
#include <stdio.h>
#include <stdlib.h>

void
swap_init ()
{
	swap_block = disk_get_role (BLOCK_SWAP);
}

void
swap_in (struct spage_entry *spe)
{
	size_t index = spte->index;
	size_t i;
	void *vaddr = spe->vaddr;

	for(i=0;i<;i++)
	{
		disk_read (swap_disk, 디스크 공간, 메모리 공간);
	}

	bitmap_flip (swap_table, index);
}

void
swap_out (struct spage_entry *spe)
{
	
}