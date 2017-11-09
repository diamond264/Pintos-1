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
	swap_block = block_get_role (BLOCK_SWAP);
}

void
swap_in (struct spage_entry *spe)
{

}

void
swap_out (struct spage_entry *spe)
{
	
}