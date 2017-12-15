<<<<<<< HEAD
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
=======
#include <stdlib.h>
#include <stdio.h>
#include "filesys/cache.h"

extern struct disk *filesys_disk;

void init_buff_cache() {
	list_init (&buff_list);
	sema_init (&sema_cache, 1);
}

void destory_buff_cache() {
	sema_down(&sema_cache);

	while (!list_empty (&buff_list)) {
		struct list_elem *first_elem = list_pop_front (&buff_list);
		struct buffer *bf = list_entry (first_elem, struct buffer, elem);
>>>>>>> origin/PJ-4

		free_buff (bf);
	}

<<<<<<< HEAD
	hand = NULL;
}

void free_buff(struct buffer *bf) {
	if (bf->dirty) {
		// 수정해야 함
		// 섹터 사이즈만큼 쓰도록
		disk_write (filesys_disk, bf->index, bf->addr);
=======
	sema_up(&sema_cache);
}

// cache list에서 빼는 작업은 안한다.
// 그래서 sema가 필요없다.
void free_buff(struct buffer *bf) {
	// Write Back
	if (bf->dirty) {
		buffer_write_back(bf);
		//disk_write (filesys_disk, bf->index, bf->addr);
>>>>>>> origin/PJ-4
	}

	free (bf->addr);
	free (bf);
}

void free_buff_with_elem(struct list_elem *bf_elem)
{
	struct buffer *bf = list_entry(bf_elem, struct buffer, elem);
	free_buff(bf);
}

<<<<<<< HEAD
=======
void buffer_write_back(struct buffer *bf)
{
	disk_write (filesys_disk, bf->index, bf->addr);
}


/*
	access 할 때 마다 캐시에서 버퍼를 검색하는건 비효율적이지 않을까?
	생각해보자.
*/
>>>>>>> origin/PJ-4
void access_buff_cache(enum cache_access access, disk_sector_t index, void *addr, off_t offset, off_t size) {
	struct list_elem *iter;
	struct buffer *bf = NULL;

<<<<<<< HEAD
=======
	sema_down(&sema_cache);

>>>>>>> origin/PJ-4
	for(iter = list_begin(&buff_list); iter != list_end(&buff_list); iter = list_next(iter)) {
		bf = list_entry(iter, struct buffer, elem);
	    if(bf->index == index){
	    	break;
	    }
	    bf = NULL;
	}

<<<<<<< HEAD
	if (bf == NULL) {
=======
	if (bf == NULL || iter == list_end(&buff_list)) {
>>>>>>> origin/PJ-4
		bf = malloc (sizeof (struct buffer));
		bf->addr = malloc (DISK_SECTOR_SIZE);
		bf->index = index;
		bf->access = false;
		bf->dirty = false;

		insert_buff(bf);

<<<<<<< HEAD
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

=======
>>>>>>> origin/PJ-4
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
<<<<<<< HEAD
	else ASSERT(0);
=======

	sema_up(&sema_cache);
>>>>>>> origin/PJ-4
}

void insert_buff(struct buffer *bf)
{
	int cache_size = get_cache_size();

	if(cache_size < MAX_CACHE_SIZE)
	{
		list_push_back(&buff_list, &bf->elem);
<<<<<<< HEAD

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

=======
	}
	else
	{
		struct list_elem *hand = list_pop_front(&buff_list);
		struct buffer *victim = list_entry(hand, struct buffer, elem);

		list_push_back(&buff_list, &bf->elem);
		free_buff(victim);
		/*if(iter->access)
		{
			iter->access = 0;

			if(hand == list_end(&buff_list)) hand = list_front(&buff_list);
			else hand = list_next(hand);
		}
		else
		{
			list_insert(hand, &bf->elem);
			list_remove(hand);
			free_buff_with_elem(hand);
			hand = &bf->elem;
			break;
		}*/
	}
}

void read_buff(disk_sector_t index, void *addr, off_t offset, off_t size)
{
	access_buff_cache(READ, index, addr, offset, size);
}

void write_buff(disk_sector_t index, void *addr, off_t offset, off_t size)
{
	access_buff_cache(WRITE, index, addr, offset, size);
}

>>>>>>> origin/PJ-4
int get_cache_size()
{
	return list_size(&buff_list);
}