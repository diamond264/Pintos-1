#ifndef CACHE_H
#define CACHE_H

#include "devices/disk.h"
#include "threads/synch.h"
#include "filesys/off_t.h"
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
	struct list_elem elem; // entry

	bool access; // 접근하면 1
	bool dirty; // 쓰면 1

	disk_sector_t index; // sector number
	void *addr; // sector data가 저장된 memory 상 주소
};

void init_buff_cache();
void destory_buff_cache();
void free_buff(struct buffer *bf);
void free_buff_with_elem(struct list_elem *bf_elem);
void buffer_write_back(struct buffer *bf);
void access_buff_cache(enum cache_access access, disk_sector_t index, void *addr, off_t offset, off_t size);
void insert_buff(struct buffer *bf);
void read_buff(disk_sector_t index, void *addr, off_t offset, off_t size);
void write_buff(disk_sector_t index, void *addr, off_t offset, off_t size);
int get_cache_size();

#endif