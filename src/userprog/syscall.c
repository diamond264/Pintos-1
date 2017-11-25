#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/off_t.h"
#include "threads/vaddr.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "vm/frame.h"

static void syscall_handler (struct intr_frame *);

struct lock file_lock;
extern struct lock page_lock;

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

struct file_elem* 
get_file_elem (int fd) {
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

    case SYS_MMAP :
     argv[0] = get_argument (sp);
     argv[1] = get_argument (sp+1);
     f->eax = syscall_mmap ((int) *argv[0], (void *) *argv[1]);
    break;

    case SYS_MUNMAP :
      argv[0] = get_argument (sp);
      syscall_unmap ((int) *argv[0]);
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

    while(!list_empty(&curr->mmap_list))
    {
      m_elem = list_pop_front(&curr->mmap_list);
      m = list_entry(m_elem, struct mmap, elem);
      syscall_unmap(m->mapid);
      free(m);
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
  validate_addr ((void *) file);
  return filesys_remove (file);
}

int
syscall_open (const char *file) {
  validate_addr ((void *) file);

  struct thread *t = thread_current ();
  struct file_elem *f;
  f = (struct file_elem*)malloc (sizeof *f);

  lock_acquire(&file_lock);
  f->file = filesys_open (file);
  lock_release(&file_lock);
  if (!f->file) {
    free (f);
    return -1;
  }

  f->fd = t->next_fd;
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

  int value = -1;

  if (fd == 0) syscall_exit (-1);

  if (fd == 1) {
    putbuf (buffer, size);
    value = size;
  } else {
    if (get_file (fd) == NULL)
    {
      syscall_exit (-1);
    }
    lock_acquire (&file_lock);
    value  = file_write(get_file(fd), buffer, (off_t) size);
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
    syscall_exit (-1);
  
  list_remove (&f_elem->elem);

  lock_acquire (&file_lock);
  file_close (f);
  lock_release (&file_lock);

  free (f_elem);

}

int syscall_mmap (int fd, void *addr)
{
  if (fd == 1 || fd == 0 || !addr || pg_ofs(addr) != 0)
    return -1;

  // addr이 이미 프레임과 매핑되어 있으면 return -1
  struct file *f = get_file(fd);

  lock_acquire(&file_lock);
  unsigned length = file_length (f);
  f = file_reopen(f);
  lock_release(&file_lock);

  if (!is_user_vaddr (addr + length) || length == 0 || f == NULL)
    return -1;

  struct mmap *map_file = (struct mmap*)malloc(sizeof (struct mmap));

  void *fp;
  // 파일 사이즈만큼 페이지가 늘어날 때 까지 for문 돌면서 할당
  lock_acquire(&page_lock);
  for(fp = addr; fp <= addr + length - PGSIZE; fp += PGSIZE)
  {
    struct spage *spe = spage_create(fp, LAZY, true);
    spe->fd = fd;
    spe->offset = fp - addr;
  }

  if(length % PGSIZE != 0) // 파일이 페이지 사이즈만큼 안떨어지고 조금 삐져나오면
  {
    struct spage *spe = spage_create(fp, LAZY, true);
    spe->fd = fd;
    spe->offset = fp - addr;
    spe->is_over = true;
    spe->length_over = length % PGSIZE;
  }
  lock_release(&page_lock);
  
  map_file->fd = fd;
  map_file->addr = addr;
  map_file->size = length;
  map_file->mapid = thread_current()->next_mapid;

  list_push_back(&thread_current()->mmap_list, &map_file->elem);

  return map_file->mapid;
}

void syscall_unmap (int mapid)
{
  struct thread *curr = thread_current ();
  struct list_elem *iter;
  struct mmap *mapping;

  for(iter = list_begin(&curr->mmap_list); iter != list_end(&curr->mmap_list); iter = list_next(iter))
  {
    mapping = list_entry(iter, struct mmap, elem);

    if (mapping->mapid == mapid)
    {
      list_remove (iter);
      break;
    }
  }

  if (mapping == NULL)
    syscall_exit (-1);

  // 여기서 뭐 해야함
  unsigned size = mapping->size;
  void *fp = mapping->addr;

  struct file *targetFile;

  lock_acquire(&page_lock);
  int write_len = PGSIZE;
  while(size > 0)
  {
    if(size - PGSIZE < 0) write_len = size;
    struct spage *spe = find_spage(fp);
    struct frame *f = find_frame(spe);

    if(pagedir_is_dirty(curr->pagedir, spe->vaddr))
    {
      targetFile = get_file(mapping->fd);
      lock_acquire(&file_lock);
      file_write_at(targetFile, f->addr, write_len, spe->offset);
      lock_release(&file_lock);

      pagedir_clear_page (curr->pagedir, spe->vaddr);
      spage_free(spe);
      frame_free(f);
    }

    size -= PGSIZE;
    fp += PGSIZE;
  }

  lock_release(&page_lock);

  syscall_close(mapping->fd);

  //file_close (mapping->file);
  free (mapping);
}


