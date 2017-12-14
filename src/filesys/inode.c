#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "filesys/cache.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* Returns the number of sectors to allocate for an inode SIZE
bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
    return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

/* Returns the disk sector that contains byte offset POS within
INODE.
Returns -1 if INODE does not contain data for a byte at offset
POS. */
static disk_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
    ASSERT (inode != NULL);
    if (pos < inode->data.length)
    {
        // return inode->data.start + pos / DISK_SECTOR_SIZE;
        off_t idx;
        int inode_size = MAX_DIRECT * MAX_INDIRECT; // 100 direct inodes, 100 inderect inodes
        struct inode_disk disk_inode = inode->data;
        struct inode_disk *temp_inode;
        temp_inode = calloc (1, sizeof *temp_inode);

        idx = pos / DISK_SECTOR_SIZE / inode_size;
        read_buff (disk_inode.start[idx], temp_inode, 0, DISK_SECTOR_SIZE);
        //printf("idx 1 : %d\n", idx);

        idx = pos / DISK_SECTOR_SIZE % inode_size;
        idx = idx / 100;
        //printf("idx 2 : %d\n", idx);
        read_buff (temp_inode->start[idx], temp_inode, 0, DISK_SECTOR_SIZE);

        idx = pos / DISK_SECTOR_SIZE % inode_size;
        idx = idx % 100;
        //printf("idx 3 : %d\n", idx);
        disk_sector_t sector = temp_inode->start[idx];

        free (temp_inode);
        return sector;
    }
    else
    {
        return -1;
    }
}

/* List of open inodes, so that opening a single inode twice
returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void inode_init (void) 
{
    list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
writes the new inode to sector SECTOR on the file system
disk.
Returns true if successful.
Returns false if memory or disk allocation fails. */
bool
inode_create (disk_sector_t sector, off_t length, int depth, int is_dir)
{
    struct inode_disk *disk_inode = NULL;
    bool success = false;
    int i;

    ASSERT (length >= 0);

    /* If this assertion fails, the inode structure is not exactly
    one sector in size, and you should fix that. */
    ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);

    disk_inode = calloc (1, sizeof *disk_inode);
    if (disk_inode != NULL)
    {
        size_t sectors = bytes_to_sectors (length);
        //printf("inode_create sector=%d, sectors=%d, length=%d, depth=%d\n", sector, sectors, length, depth);
        disk_inode->length = length;
        disk_inode->magic = INODE_MAGIC;
        disk_inode->depth = depth;
        disk_inode->is_dir = is_dir;

        if(disk_inode->depth == 0) // direct
        {
            if(free_map_allocate(sectors, disk_inode->start))
            {
                if(sectors > 0)
                {
                    static char zeros[DISK_SECTOR_SIZE];

                    for(i=0; i<sectors; i++)
                    {
                        write_buff(disk_inode->start[i], zeros, 0, DISK_SECTOR_SIZE);
                    }

                    success = true;
                }
            }
        }
        else
        {
            size_t indirect_sectors = 0;

            if(disk_inode->depth == LV1)
            {
                indirect_sectors = DIV_ROUND_UP(sectors, MAX_DIRECT);
            }
            else if(disk_inode->depth == LV2)
            {
                indirect_sectors = DIV_ROUND_UP(sectors, MAX_DIRECT * MAX_INDIRECT);
            }

            if(free_map_allocate(indirect_sectors, disk_inode->start))
            {
                for(i=0; i<indirect_sectors; i++)
                {
                    size_t need_sectors = 0;
                    int div = 0;
                    if(i == indirect_sectors - 1) // 마지막 섹터면
                    {
                        div = depth == 1 ? MAX_DIRECT : MAX_DIRECT * MAX_INDIRECT;
                        need_sectors = sectors % div;
                    }
                    else
                    {
                        need_sectors = depth == 1 ? MAX_DIRECT : MAX_DIRECT * MAX_INDIRECT;
                    }

                    bool created = inode_create(disk_inode->start[i], need_sectors * DISK_SECTOR_SIZE, depth-1, is_dir);
                    if(!created)
                    {
                        int j;
                        for(j=0; j<i; j++)
                        {
                            inode_disk_remove(disk_inode->start[j]);
                        }
                        break;
                    }
                }

                success = true;
            }
        }

        if(success)
        {
            write_buff(sector, disk_inode, 0, DISK_SECTOR_SIZE);
        }

        free (disk_inode);
    }

    return success;
}

/* Reads an inode from SECTOR
and returns a `struct inode' that contains it.
Returns a null pointer if memory allocation fails. */
struct inode* inode_open (disk_sector_t sector) 
{
    struct list_elem *e;
    struct inode *inode;

    /* Check whether this inode is already open. */
    for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
        e = list_next (e)) 
    {
        inode = list_entry (e, struct inode, elem);
        if (inode->sector == sector) 
        {
            inode_reopen (inode);
            return inode; 
        }
    }

    /* Allocate memory. */
    inode = malloc (sizeof *inode);
    if (inode == NULL)
        return NULL;

    /* Initialize. */
    list_push_front (&open_inodes, &inode->elem);
    inode->sector = sector;
    inode->open_cnt = 1;
    inode->deny_write_cnt = 0;
    inode->removed = false;
    sema_init(&inode->sema_inode, 1);

    //disk_read (filesys_disk, inode->sector, &inode->data);
    read_buff(inode->sector, &inode->data, 0, DISK_SECTOR_SIZE);
    return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
    if (inode != NULL)
        inode->open_cnt++;
    return inode;
}

/* Returns INODE's inode number. */
disk_sector_t
inode_get_inumber (const struct inode *inode)
{
    return inode->sector;
}

/* Closes INODE and writes it to disk.
If this was the last reference to INODE, frees its memory.
If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
    /* Ignore null pointer. */
    if (inode == NULL)
        return;

    /* Release resources if this was the last opener. */
    if (--inode->open_cnt == 0)
    {
        /* Remove from inode list and release lock. */
        list_remove (&inode->elem);

        /* Deallocate blocks if removed. */
        if (inode->removed) 
        {
            /*free_map_release (inode->sector, 1);
            free_map_release (inode->data.start, bytes_to_sectors (inode->data.length)); */
            inode_disk_remove(inode->sector);
        }

        free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
has it open. */
void
inode_remove (struct inode *inode) 
{
    ASSERT (inode != NULL);
    inode->removed = true;
}

void inode_disk_remove(disk_sector_t sector)
{
    struct inode_disk *di = calloc(1, sizeof(struct inode_disk));
    read_buff(sector, di, 0, DISK_SECTOR_SIZE);
    size_t sectors = bytes_to_sectors(di->length);

    free_map_release(&sector, 1);

    if(di->depth == 0) // direct inode_disk
    {
        free_map_release(di->start, sectors);
    }
    else
    {
        int i;
        for(i=0; i<sectors; i++)
        {
            inode_disk_remove(di->start[i]);
        }
    }

    free(di);
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
Returns the number of bytes actually read, which may be less
than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
    uint8_t *buffer = buffer_;
    off_t bytes_read = 0;

    if(inode->data.length <= offset) return 0;

    while (size > 0)
    {
        /* Disk sector to read, starting byte offset within sector. */
        disk_sector_t sector_idx = byte_to_sector (inode, offset);
        //printf("sector : %d\n", sector_idx);
        int sector_ofs = offset % DISK_SECTOR_SIZE;

        /* Bytes left in inode, bytes left in sector, lesser of the two. */
        off_t inode_left = inode_length (inode) - offset;
        int sector_left = DISK_SECTOR_SIZE - sector_ofs;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        /* Number of bytes to actually copy out of this sector. */
        int chunk_size = size < min_left ? size : min_left;
        //printf("size : %d, min_left : %d, chunk : %d\n", size, min_left, chunk_size);
        if (chunk_size <= 0)
        {
            break;
        }

        //printf("remain size : %d, sector_idx : %d, buffer + bytes_reads : %p, sector_ofs : %d, chunk_size : %d\n", size, sector_idx, buffer + bytes_read, sector_ofs, chunk_size);
        read_buff(sector_idx, buffer + bytes_read, sector_ofs, chunk_size);

        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_read += chunk_size;
    }

    //printf("return %d\n", bytes_read);

    return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
Returns the number of bytes actually written, which may be
less than SIZE if end of file is reached or an error occurs.
(Normally a write at end of file would extend the inode, but
growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
    off_t offset) 
{
    const uint8_t *buffer = buffer_;
    off_t bytes_written = 0;

    if (inode->deny_write_cnt)
        return 0;

    if(!inode_expand(&inode->data, inode->sector, offset + size, LV2, inode->data.is_dir)) return 0;

    while (size > 0) 
    {
        /* Sector to write, starting byte offset within sector. */
        disk_sector_t sector_idx = byte_to_sector (inode, offset);
        int sector_ofs = offset % DISK_SECTOR_SIZE;

        /* Bytes left in inode, bytes left in sector, lesser of the two. */
        off_t inode_left = inode_length (inode) - offset;
        int sector_left = DISK_SECTOR_SIZE - sector_ofs;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        /* Number of bytes to actually write into this sector. */
        int chunk_size = size < min_left ? size : min_left;
        if (chunk_size <= 0)
            break;

        write_buff(sector_idx, buffer + bytes_written, 0, chunk_size);

        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_written += chunk_size;
    }

    return bytes_written;
}

bool inode_expand(struct inode_disk *disk_inode, disk_sector_t sector, off_t length, int depth, int is_dir)
{
    if(length > disk_inode->length)
    {
        bool success = false;
        int i;
        ASSERT (length >= 0);

        /* If this assertion fails, the inode structure is not exactly
        one sector in size, and you should fix that. */
        ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);
        ASSERT (disk_inode != NULL);

        size_t sectors = bytes_to_sectors (length); // bytes 길이를 필요한 sector 수로 바꿔준다.
        size_t old_sectors = bytes_to_sectors (disk_inode->length);
        disk_inode->length = length;
        disk_inode->depth = depth;
        disk_inode->is_dir = is_dir;

        int more_sectors = 0;

        if(disk_inode->depth == 0) // direct
        {
            more_sectors = sectors - old_sectors;
            if(free_map_allocate(more_sectors, disk_inode->start + old_sectors))
            {
                if(more_sectors > 0)
                {
                    static char zeros[DISK_SECTOR_SIZE];
                    int i;

                    for(i=old_sectors; i<sectors; i++)
                    {
                        write_buff(disk_inode->start[i], zeros, 0, DISK_SECTOR_SIZE);
                    }
                }
            }

            success = true;
        }
        else
        {
            size_t indirect_sectors = 0;
            size_t old_indirect_sectors = 0;
            size_t more_sector_size = sectors - old_sectors; // 현재 끝부분 섹터에서 실제로 늘려야 하는 하위 섹터의 개수를 의미.
            // 205개에서 232개까지 sector를 늘려야 한다면, more_sector_size는 27개.
            // 205개에서 313개까지 sector를 늘려야 한다면, more_sector_size는 100개. (마지막 섹터는 300개까지만 늘어날 수 있으므로)

            if(disk_inode->depth == 1)
            {
                indirect_sectors = DIV_ROUND_UP(sectors, MAX_DIRECT);
                old_indirect_sectors = DIV_ROUND_UP(old_sectors, MAX_DIRECT);

                if(more_sector_size > MAX_DIRECT) more_sector_size = MAX_DIRECT;
            }
            else if(disk_inode->depth == 2)
            {
                indirect_sectors = DIV_ROUND_UP(sectors, MAX_DIRECT * MAX_INDIRECT);
                old_indirect_sectors = DIV_ROUND_UP(old_sectors, MAX_DIRECT * MAX_INDIRECT);

                if(more_sector_size > MAX_DIRECT * MAX_INDIRECT) more_sector_size = MAX_DIRECT * MAX_INDIRECT;
            }

            more_sectors = indirect_sectors - old_indirect_sectors;// 더 늘려야 하는 섹터 수

            if(old_indirect_sectors > 0) // 마지막 섹터에 있는 데이터를 복사한 후, expand된 부분에는 0 넣어준다.
            {
                struct inode_disk *more_di = calloc(1, sizeof(struct inode_disk));
                // old_indirect_sectors-1번째 섹터를 읽은 후에, expand해준다.
                read_buff(disk_inode->start[old_indirect_sectors-1], more_di, 0, DISK_SECTOR_SIZE);
                inode_expand(more_di, disk_inode->start[old_indirect_sectors-1], more_sector_size * DISK_SECTOR_SIZE, depth - 1, is_dir);

                free(more_di);
            }

            if(free_map_allocate(more_sectors, disk_inode->start + old_indirect_sectors))
            {
                for(i=old_indirect_sectors; i<indirect_sectors; i++)
                {
                    size_t need_sectors = 0;
                    int div = 0;
                    if(i == indirect_sectors - 1) // 마지막 섹터면
                    {
                        div = depth == 1 ? MAX_DIRECT : MAX_DIRECT * MAX_INDIRECT;
                        need_sectors = sectors % div;
                    }
                    else
                    {
                        need_sectors = depth == 1 ? MAX_DIRECT : MAX_DIRECT * MAX_INDIRECT;
                    }

                    success = inode_create(disk_inode->start[i], need_sectors * DISK_SECTOR_SIZE, depth-1, is_dir);
                }
            }
        }

        if(success)
        {
            // inode 정보 변경 내역을 저장한다.
            write_buff(sector, disk_inode, 0, DISK_SECTOR_SIZE);        
        }

        return success;
    }
    else return true;
}

/* Disables writes to INODE.
May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
    inode->deny_write_cnt++;
    ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
Must be called once by each inode opener who has called
inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
    ASSERT (inode->deny_write_cnt > 0);
    ASSERT (inode->deny_write_cnt <= inode->open_cnt);
    inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
    return inode->data.length;
}
