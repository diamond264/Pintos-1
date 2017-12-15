#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/off_t.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

struct lock file_lock;
<<<<<<< HEAD
extern struct lock page_lock;
extern struct semaphore page_sema;
=======
>>>>>>> origin/PJ-4

void syscall_exit (int status);
tid_t syscall_exec (const char *cmd_line);
int syscall_wait (tid_t pid);
bool syscall_create (const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
int syscall_open (const char *file);
int syscall_filesize (int fd);
int syscall_read (struct intr_frame *f, int fd, const void *buffer, unsigned size);
int syscall_write (struct intr_frame *f, int fd, const void *buffer, unsigned size);
void syscall_seek (int fd, unsigned position);
unsigned syscall_tell (int fd);
void syscall_close (int fd);
int syscall_inumber (int fd);
bool syscall_isdir (int fd);
bool syscall_readdir (int fd, char *name);
bool syscall_chdir (const char *dir);
bool syscall_mkdir (const char *dir);

uint32_t
get_argument (uint32_t *sp) {
  sp++;
  validate_addr ((void *) sp);
  return sp;
}

struct file*
get_file (int fd) {
  struct thread *curr = thread_current ();
  struct list_elem *i;

  for (i=list_begin (&curr->files); i!=list_end (&curr->files); i=list_next(i)) {
    struct file_elem *file_i = list_entry (i, struct file_elem, elem);
    if (file_i->fd == fd) {
      return file_i->file;
    }
  }

  return NULL;
}

struct file_elem* get_file_elem (int fd) {
  struct thread *curr = thread_current ();
  struct list_elem *i;

  for (i=list_begin (&curr->files); i!=list_end (&curr->files); i=list_next(i)) {
    struct file_elem *file_i = list_entry (i, struct file_elem, elem);
    if (file_i->fd == fd) {
      return file_i;
    }
  }

  return NULL;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&file_lock);
}

static void
syscall_handler (struct intr_frame *f) 
{
  uint32_t *argv[5];
  uint32_t *sp = f->esp;
  validate_addr (sp);

  switch (*sp) {
    case SYS_HALT :
      power_off ();
      break;

    case SYS_EXIT :
      argv[0] = get_argument (sp);
      syscall_exit ((int) *argv[0]);
      break;

    case SYS_EXEC :
      argv[0] = get_argument (sp);
      f->eax = syscall_exec ((char *) *argv[0]);
      break;

    case SYS_WAIT :
      argv[0] = get_argument (sp);
      f->eax = syscall_wait ((tid_t) *argv[0]);
      break;

    case SYS_CREATE :
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      f->eax = syscall_create ((char *)*(uint32_t*)argv[0], (unsigned) *argv[1]);
      break;

    case SYS_REMOVE :
      argv[0] = get_argument (sp);
      f->eax = syscall_remove ((char *) *argv[0]);
      break;

    case SYS_OPEN :
      argv[0] = get_argument (sp);
      f->eax = syscall_open ((char *) *(uint32_t *)argv[0]);
      break;

    case SYS_FILESIZE :
      argv[0] = get_argument (sp);
      f->eax = syscall_filesize ((int) *argv[0]);
      break;

    case SYS_READ :
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      argv[2] = get_argument (sp+2);
      f->eax = (off_t) syscall_read (f, (int) *argv[0], (void *) *argv[1], (unsigned) *argv[2]);
      break;

    case SYS_WRITE :
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      argv[2] = get_argument (sp+2);
      f->eax = (off_t) syscall_write (f, (int) *argv[0], (void *) *argv[1], (unsigned) *argv[2]);
      break;

    case SYS_SEEK :
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      syscall_seek ((int) *argv[0], (unsigned) *argv[1]);
      break;

    case SYS_TELL :
      argv[0] = get_argument (sp);
      f->eax = syscall_tell ((int) *argv[0]);
      break;

    case SYS_CLOSE  :
      argv[0] = get_argument (sp);
      syscall_close ((int) *argv[0]);
      break;

    case SYS_CHDIR :
      argv[0] = get_argument (sp);
      f->eax = syscall_chdir ((const char *) *argv[0]);
      break;

    case SYS_MKDIR :
      argv[0] = get_argument (sp);
      f->eax = syscall_mkdir ((const char *) *argv[0]);
      break;

    case SYS_READDIR : 
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      f->eax = syscall_readdir ((int) *argv[0], (char *) *argv[1]);
      break;

    case SYS_ISDIR : 
      argv[0] = get_argument (sp);
      f->eax = syscall_isdir ((int) *argv[0]);
      break;

    case SYS_INUMBER :
      argv[0] = get_argument (sp);
      f->eax = syscall_inumber ((int) *argv[0]);
      break;

    default :
      break;
  }
}

void
syscall_exit (int status) {
  struct thread *curr;
  curr = thread_current ();
  curr->exit_status = status;

  struct list_elem *t_elem;
    struct child_elem *t;

    struct list_elem *f_elem;
    struct file_elem *f;

    struct list_elem *m_elem;
    struct mmap *m;

  while (!list_empty (&curr->children)) {
      t_elem = list_pop_front (&curr->children);
      t = list_entry (t_elem, struct child_elem, elem);
      free (t);
    }

    while (!list_empty (&curr->files)) {
      f_elem = list_pop_front (&curr->files);
      f = list_entry (f_elem, struct file_elem, elem);
      free (f->file);
      free (f);
    }

  thread_exit ();
}

tid_t
syscall_exec (const char *cmd_line) {
  validate_addr ((void *) cmd_line);
  return (tid_t) process_execute (cmd_line);
}

int
syscall_wait (tid_t pid) {
  return process_wait (pid);
}

bool
syscall_create (const char *file, unsigned initial_size) {
  validate_addr ((void *) file);
  lock_acquire(&file_lock);
  bool res = filesys_create (file, initial_size);
  lock_release(&file_lock);
  return res;
}

bool
syscall_remove (const char *file) {
  lock_acquire(&file_lock);
  validate_addr ((void *) file);
  bool success = filesys_remove (file);
  lock_release(&file_lock);
  return success;
}

int
syscall_open (const char *filename) {
  validate_addr ((void *) filename);

  struct thread *t = thread_current ();
  struct file_elem *f;
  f = malloc (sizeof (struct file_elem));

  lock_acquire(&file_lock);
  f->file = filesys_open (filename);
  lock_release(&file_lock);
  if (f->file == NULL) {
    return -1;
  }

  struct inode *inode = file_get_inode (f->file);
  if (inode->data.is_dir == DIR) f->dir = dir_open (inode_reopen (inode));
  else f->dir = NULL;

  f->fd = t->next_fd;
  //printf("%s %d\n", filename, strlen(filename));
  strlcpy(f->name, filename, strlen(filename) + 1); // filename 저장
  //printf("%s\n", f->name);
  t->next_fd++;
  list_push_back (&t->files, &f->elem);

  return f->fd;
}

int
syscall_filesize (int fd) {
  lock_acquire(&file_lock);
  int length = file_length (get_file (fd));
  lock_release(&file_lock);

  return length;
}


int
syscall_read (struct intr_frame *f, int fd, const void *buffer, unsigned size) {
  if ((void *) buffer == NULL)
  {
    syscall_exit (-1);
  }
  validate_addr_syscall (f, (void *) buffer);
  validate_addr_syscall (f, (void *)(buffer + size));

  if (fd == 1) syscall_exit (-1);

  int value = -1;
  int i;


  if (fd == 0) {
    for (i = 0; i < (int) size; i++) {

      *(uint8_t *)buffer = input_getc();
      buffer++;
      validate_addr_syscall (f, (void *) buffer);
      if ((void *) buffer == NULL)
      {
        syscall_exit (-1);
      }
    }
    value = size;
  } else {
    /// if no file, exit
    if (get_file (fd) == NULL)
    {
      syscall_exit (-1);
    }
    lock_acquire (&file_lock);
    value  = file_read(get_file(fd), buffer, (off_t) size);
    lock_release(&file_lock);
  }
  return value;
}

int
syscall_write (struct intr_frame *f, int fd, const void *buffer, unsigned size) {
  if ((void *) buffer == NULL)
  {
    syscall_exit (-1);
  }
  validate_addr_syscall (f, (void *) buffer);
  validate_addr_syscall (f, (void *)(buffer + size));
  validate_addr((void*)buffer);
  validate_addr((void*)(buffer + size)); 

  int value = -1;

  if (fd == 0) syscall_exit (-1);

  if (fd == 1) {
    putbuf (buffer, size);
    value = size;
  } else {
    struct file *targetFile = get_file(fd);
    if (targetFile == NULL) syscall_exit (-1);
    if (targetFile->inode->data.is_dir == DIR) return -1;

    lock_acquire (&file_lock);
    value  = file_write(targetFile, buffer, (off_t) size);
    lock_release(&file_lock);
  }
  return value;
}

void
syscall_seek (int fd, unsigned position) {
  if (fd ==1 || fd == 0)
    syscall_exit (-1);

  struct file *f = get_file (fd);
  if (f == NULL) syscall_exit (-1);

  lock_acquire (&file_lock);
  file_seek (f, position);
  lock_release (&file_lock);
}

unsigned
syscall_tell (int fd) {
  if (fd ==1 || fd == 0)
    syscall_exit (-1);

  struct file *f = get_file (fd);
  if (f == NULL) syscall_exit (-1);

  lock_acquire (&file_lock);
  unsigned res = file_tell (f);
  lock_release (&file_lock);

  return res;
}

void
syscall_close (int fd) {
  if (fd == 1 || fd == 0)
    syscall_exit (-1);

  struct file_elem *f_elem;
  struct file *f;

  f_elem = get_file_elem (fd);
  f = get_file (fd);

  if (f == NULL && f_elem != NULL)
    free (f_elem);

  if (f == NULL || f_elem == NULL) 
  {
    syscall_exit (-1);
  }
  
  list_remove (&f_elem->elem);

  lock_acquire (&file_lock);
  file_close (f);
  if(f_elem->dir != NULL)
    dir_close (f_elem->dir);
  lock_release (&file_lock);

  free (f_elem);

}


bool syscall_chdir (const char *dir)
{
  lock_acquire(&file_lock);
<<<<<<< HEAD
  struct file *mapped_file = filesys_open(get_file_elem(fd)->name);
  int length = file_length(mapped_file);
  lock_release(&file_lock);

  if (!is_user_vaddr (addr + length) || length == 0 || mapped_file == NULL)
    return -1;

  struct mmap *mapping = (struct mmap*)malloc(sizeof (struct mmap));

  void *fp;
  // 파일 사이즈만큼 페이지가 늘어날 때 까지 for문 돌면서 할당
  //lock_acquire(&page_lock);
  //sema_down(&page_sema);

  for(fp = addr; fp <= addr + length - PGSIZE; fp += PGSIZE)
  {
    struct spage *spe = spage_create(fp, LAZY, true);
=======
>>>>>>> origin/PJ-4

  struct inode *inode;
  struct dir *curr;

  curr = parse_directory (dir, false);
  if (curr == NULL)
  {
    lock_release(&file_lock);
    return false;
  }
<<<<<<< HEAD
  //lock_release(&page_lock);
  
  mapping->file = mapped_file;
  mapping->addr = addr;
  mapping->size = length;
  mapping->mapid = curr->next_mapid++;
  mapping->owner = curr;
=======
>>>>>>> origin/PJ-4

  if (thread_current ()->curr_dir) dir_close (thread_current ()->curr_dir);
  thread_current ()->curr_dir = curr;

<<<<<<< HEAD
  //sema_up(&page_sema);

  return mapping->mapid;
=======
  lock_release(&file_lock);
  return true;
>>>>>>> origin/PJ-4
}

bool syscall_mkdir (const char *dir)
{
  lock_acquire(&file_lock);

  if(strlen(dir) == 0)
  {
    return false;
  }
  if(dir == '\0') return false;

  struct dir *parent_dir = parse_directory(dir, true);
  if(parent_dir == NULL){
    lock_release(&file_lock);
    return false;
  }

  char *new_dir_name = parse_name(dir);
  if(strlen(new_dir_name) == 0)
  {
    dir_close(parent_dir);
    free(new_dir_name);
    lock_release(&file_lock);
    return false;
  }

  bool success = true;

<<<<<<< HEAD
  //lock_acquire(&page_lock);
  
  int write_len = PGSIZE;
  while(size > 0)
  {
    sema_down(&page_sema);
    if(size - PGSIZE < 0) write_len = size;
    struct spage *spe = find_spage(fp);
=======
  disk_sector_t inode_sector = -1;
  struct inode *inode;
  if(!dir_lookup(parent_dir, new_dir_name, &inode)
    && free_map_allocate (1, &inode_sector)
    && dir_create(inode_sector, 16)
    && dir_add (parent_dir, new_dir_name, inode_sector));
  else
  {
    //free_map_release(&inode_sector, 1);
    return false;
  }
>>>>>>> origin/PJ-4

  struct inode *pinode = dir_get_inode(parent_dir);
  dir_set_parent (inode_sector, pinode->sector);

  dir_close(parent_dir);
  free(new_dir_name);

  lock_release(&file_lock);
  
  return success;
}

bool syscall_readdir (int fd, char *name)
{
  struct file_elem *f = get_file_elem (fd);
  if(f == NULL) return false;

  struct dir *dir = f->dir;

<<<<<<< HEAD
    size -= PGSIZE;
    fp += PGSIZE;
    sema_up(&page_sema);
  }

  //lock_release(&page_lock);
=======
  if (dir == NULL) return false;
  if (f->file == NULL) return false;

  lock_acquire(&file_lock);
  bool rv = dir_readdir (dir, name);
  lock_release(&file_lock);
>>>>>>> origin/PJ-4

  return rv;
}

bool syscall_isdir (int fd)
{
  struct file_elem *f = get_file_elem (fd);
  if (f->file == NULL) return false;
  if (f->dir == NULL) return false;
  return true;
}

int syscall_inumber (int fd)
{
  struct file *file = get_file (fd);
  if (file == NULL) return -1;
  struct inode *inode = file_get_inode(file);

  return inode->sector;
}

