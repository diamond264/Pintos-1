#include "filesys/cache.h"

void init_buff_cache() {
	list_init (&buff_list);
	sema_init (&sema_cache, 1);
	hand = NULL;
}

void free_buff_cache() {
	while (!list_empty (&buff_list)) {
		struct list_entry *elem = list_pop_front (&buff_list);
		struct buffer *bf = list_entry (elem, struct buffer, elem);

		free_buff (bf);
	}

	hand = NULL;
}

void free_buff(struct buffer *bf) {
	if (bf->dirty) {
		// 수정해야 함
		// 섹터 사이즈만큼 쓰도록
		disk_write (filesys_disk, bf->index, bf->addr);
	}

	free (bf->addr);
	free (bf);
}

void free_buff_with_elem(struct list_elem *bf_elem)
{
	struct buffer *bf = list_entry(bf_elem, struct buffer, elem);
	free_buff(bf);
}

void access_buff_cache(enum cache_access access, disk_sector_t index, void *addr, off_t offset, off_t size) {
	struct list_elem *iter;
	struct buffer *bf = NULL;

	for(iter = list_begin(&buff_list); iter != list_end(&buff_list); iter = list_next(iter)) {
		bf = list_entry(iter, struct buffer, elem);
	    if(bf->index == index){
	    	break;
	    }
	    bf = NULL;
	}

	if (bf == NULL) {
		bf = malloc (sizeof (struct buffer));
		bf->addr = malloc (DISK_SECTOR_SIZE);
		bf->index = index;
		bf->access = false;
		bf->dirty = false;

		insert_buff(bf);

		/*if (hand == NULL) { // 캐시가 비어있던 경우
			list_push_back (&buff_list, &bf->elem);
			//if (list_size (&buff_list) == CACHE_FULL)
			hand = list_begin (&buff_list);
		}
		else {
			evict_buff_cache ();
			list_insert (hand, &bf->elem);
			hand = list_next(hand);
		}*/

		disk_read (filesys_disk, bf->index, bf->addr);
	}

	bf->access = true;

	if (access == READ) {
		memcpy(addr, bf->addr+offset, size);
	}
	else if (access == WRITE) {
		bf->dirty = true;
		memcpy(bf->addr+offset, addr, size);
	}
	else ASSERT(0);
}

void insert_buff(struct buffer *bf)
{
	int cache_size = get_cache_size();

	if(cache_size < MAX_CACHE_SIZE)
	{
		list_push_back(&buff_list, &bf->elem);

		if(hand == NULL) hand = list_front(&buff_list);
	}
	else
	{
		while(true)
		{
			struct buffer *iter = list_entry(hand, struct buffer, elem);
			if(iter->access)
			{
				iter->access = 0;

				if(hand == list_end(&buff_list)) hand = list_front(&buff_list);
				else hand = list_next(hand);
			}
			else
			{
				list_insert(hand, bf->elem);
				free_buff_with_elem(hand);
			}
		}
	}
}

int get_cache_size()
{
	return list_size(&buff_list);
}