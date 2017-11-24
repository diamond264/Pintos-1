#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/off_t.h"
#include "threads/vaddr.h"
#include "vm/page.h"

static void syscall_handler (struct intr_frame *);
struct lock file_lock;

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

    // case SYS_MMAP :
    //   argv[0] = get_argument (sp);
    //   argv[1] = get_argument (sp+1);
    //   f->eax = syscall_mmap ((int) *argv[0], (void *) *argv[1]);
    //   break;

    // case SYS_MUNMAP :
    //   argv[0] = get_argument (sp);
    //   syscall_unmap ((int) *argv[0]);
    //   break;

    default :
      break;
  }
}

void
syscall_exit (int status) {
  struct thread *t;
  t = thread_current ();
  t->exit_status = status;
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
  return filesys_create (file, initial_size);
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

  f->file = filesys_open (file);
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
  return file_length (get_file (fd));
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

  lock_acquire (&file_lock);

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
      syscall_exit (-1);
    value  = file_read(get_file(fd), buffer, (off_t) size);
  }

  lock_release(&file_lock);
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

  lock_acquire (&file_lock);

  if (fd == 1) {
    putbuf (buffer, size);
    value = size;
  } else {
    if (get_file (fd) == NULL)
    {
      lock_release(&file_lock);
      syscall_exit (-1);
    }
    value  = file_write(get_file(fd), buffer, (off_t) size);
  }

  lock_release(&file_lock);
  return value;
}

void
syscall_seek (int fd, unsigned position) {
  if (fd ==1 || fd == 0)
    syscall_exit (-1);

  struct file *f = get_file (fd);
  if (f == NULL) syscall_exit (-1);

  return file_seek (f, position);
}

unsigned
syscall_tell (int fd) {
  if (fd ==1 || fd == 0)
    syscall_exit (-1);

  struct file *f = get_file (fd);
  if (f == NULL) syscall_exit (-1);

  return file_tell (f);
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
  file_close (f);
  free (f_elem);
}

// int syscall_mmap (int fd, void *addr)
// {
//   if (fd == 1 || fd == 0
//     || !addr
//     || pg_ofs(addr) != 0)
//     return -1;

//   struct file_elem *f_elem;
//   struct file *f;

//   f_elem = get_file_elem (fd);
//   f = get_file (fd);
//   int length = file_length (f);

//   if (!is_user_vaddr (addr + length)
//     || length == 0
//     || f_elem == NULL
//     || f == NULL)
//     return -1;
  
// }

// void syscall_unmap (int mapid)
// {
//   struct thread *curr = thread_current ();
//   struct list_elem *iter;
//   struct mmap *mapping;

//   for(iter = list_begin(&curr->mmap_list); iter != list_end(&curr->mmap_list); iter = list_next(iter))
//   {
//     mapping = list_entry(iter, struct mmap, elem);

//     if (mapping->mapid == mapid)
//     {
//       list_remove (iter);
//       break;
//     }
//   }

//   if (mapping == NULL)
//     syscall_exit (-1);

//   // 여기서 뭐 해야함

//   file_close (mapping->file);
//   free (mapping);
// }


