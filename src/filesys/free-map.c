#include "filesys/free-map.h"
#include <bitmap.h>
#include <debug.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"

static struct file *free_map_file;   /* Free map file. */
static struct bitmap *free_map;      /* Free map, one bit per disk sector. */

/* Initializes the free map. */
void
free_map_init (void) 
{
  free_map = bitmap_create (disk_size (filesys_disk));
  if (free_map == NULL)
    PANIC ("bitmap creation failed--disk is too large");
  bitmap_mark (free_map, FREE_MAP_SECTOR);
  bitmap_mark (free_map, ROOT_DIR_SECTOR);
}

/* Allocates CNT consecutive sectors from the free map and stores
   the first into *SECTORP.
   Returns true if successful, false if all sectors were
   available. */
bool
free_map_allocate (size_t cnt, disk_sector_t *sectorp) 
{
  // 원래는 cnt만큼의 연속된 false공간을 찾아 할당한다
  // allocation을 효율적으로 하기 위해 cnt개의 1개씩 빈 공간을 찾아 준다

  size_t i, j;
  disk_sector_t sector;
  for(i=0;i<cnt;i++) {
    sector = bitmap_scan_and_flip (free_map, 0, 1, false);
    if (sector == BITMAP_ERROR)
    {
      ASSERT(0);
      for (j=0;j<i-1;j++)
        bitmap_set_multiple (free_map, sectorp[j], 1, false);
      return false;
    }
    sectorp[i] = sector;
  }

  if (sector != BITMAP_ERROR
      && free_map_file != NULL
      && !bitmap_write (free_map, free_map_file))
  {
    ASSERT(0);
    for (j=0;j<cnt;j++)
        bitmap_set_multiple (free_map, sectorp[j], 1, false);
    return false;
  }

  return true;
}

/* Makes CNT sectors starting at SECTOR available for use. */
void
free_map_release (disk_sector_t *sector, size_t cnt)
{
  ASSERT (bitmap_all (free_map, *sector, cnt));
  size_t i;

  for (i=0;i<cnt;i++) {
    bitmap_set_multiple (free_map, sector[i], 1, false);
  }
  // bitmap_set_multiple (free_map, sector, cnt, false);
  bitmap_write (free_map, free_map_file);
}

/* Opens the free map file and reads it from disk. */
void
free_map_open (void) 
{
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_read (free_map, free_map_file))
    PANIC ("can't read free map");
}

/* Writes the free map to disk and closes the free map file. */
void
free_map_close (void) 
{
  file_close (free_map_file);
}

/* Creates a new free map file on disk and writes the free map to
   it. */
void
free_map_create (void) 
{
  /* Create inode. */
  if (!inode_create (FREE_MAP_SECTOR, bitmap_file_size (free_map), LV2, FILE))
    PANIC ("free map creation failed");

  /* Write bitmap to file. */
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_write (free_map, free_map_file))
    PANIC ("can't write free map");
}
