#include <stdio.h>
#include <stdlib.h>

struct frame {
	void *page;
	void *vaddr;
	struct list_elem elem;
}