#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include <list.h>
#include "threads/synch.h"
#include "filesys/off_t.h"
#include "devices/disk.h"

static const int FILE = 0;
static const int DIR = 1;

static const int LV0 = 0;
static const int LV1 = 1;
static const int LV2 = 2;

#define MAX_DIRECT 100
#define MAX_INDIRECT 100
#define MAX_DOUBLE 2

/* On-disk inode.
Must be exactly DISK_SECTOR_SIZE=512 bytes long. */
struct inode_disk
{
    disk_sector_t start[100]; // 실제 데이터 or sector pointer를 나타내는 부분. 위치 계산 쉽게 하기 위해 100 size.
  	disk_sector_t parent; // parent sector를 가리킨다.
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. 파일 포맷의 종류를 나타낸다. */
    int depth; // 0이면 root, 1이면 indirect node, 2면 doubly indirect node.
    int is_dir; // directory면 1. 아니면 0.
    // 4*100 + 4*5 = 420
    uint32_t unused[23]; // 512 bytes 할당하기 위한 offset.
};

/* In-memory inode. */
struct inode 
{
    struct list_elem elem;              /* Element in inode list. */
    disk_sector_t sector;               /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
   	struct semaphore sema_inode; // inode semaphore.
};

struct bitmap;

void inode_init (void);
bool inode_create (disk_sector_t sector, off_t length, int depth, int is_dir);
struct inode *inode_open (disk_sector_t);
struct inode *inode_reopen (struct inode *);
disk_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
bool inode_expand(struct inode_disk *disk_inode, disk_sector_t sector, off_t length, int depth, int is_dir);
void inode_disk_remove(disk_sector_t sector);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

#endif /* filesys/inode.h */
