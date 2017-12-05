#ifndef CACHE_H
#define CACHE_H

#include "devices/disk.h"
#include <list.h>

#define MAX_CACHE_SIZE 64 // 최대 64개의 섹터가 캐싱된다.

struct list buff_list;
static struct list_elem *hand;
struct semaphore sema_cache;

enum cache_access {
	READ,
	WRITE
};

struct buffer {
	struct list_elem elem;

	bool access;
	bool dirty;

	disk_sector_t index;
	void *addr;
};

void init_buff_cache();
void free_buff_cache();
void free_buff(struct buffer *bf);
void free_buff_with_elem(struct list_elem *bf_elem);
void access_buff_cache(enum cache_access access, disk_sector_t index, void *addr, off_t offset, off_t size);
void insert_buff(struct buffer *bf);
int get_cache_size();

#endif