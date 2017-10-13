#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/off_t.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
struct lock file_lock;

void syscall_exit (int status);
tid_t syscall_exec (const char *cmd_line);
int syscall_wait (tid_t pid);
bool syscall_create (const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
int syscall_open (const char *file);
int syscall_filesize (int fd);
int syscall_read (int fd, const void *buffer, unsigned size);
int syscall_write (int fd, const void *buffer, unsigned size);
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
  //printf ("system call!\n");
  uint32_t *argv[5];
  uint32_t *sp = f->esp;
  validate_addr (sp);

  switch (*sp) {
    case SYS_HALT :
      //printf ("halt call!\n");
      power_off ();
      break;

    case SYS_EXIT :
      //printf ("exit call!\n");
      argv[0] = get_argument (sp);
      //printf ("%d\n", argv[0]);
      syscall_exit ((int) *argv[0]);
      break;

    case SYS_EXEC :
      //printf("exec call!\n")
      argv[0] = get_argument (sp);
      f->eax = syscall_exec ((char *) *argv[0]);
      break;

    case SYS_WAIT :
      //printf ("wait call!\n");
      argv[0] = get_argument (sp);
      f->eax = syscall_wait ((tid_t) *argv[0]);
      break;

    case SYS_CREATE :
      //printf ("create call!\n");
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      f->eax = syscall_create ((char *)*(uint32_t*)argv[0], (unsigned) *argv[1]);
      break;

    case SYS_REMOVE :
      //printf ("remove call!\n");
      argv[0] = get_argument (sp);
      f->eax = syscall_remove ((char *) *argv[0]);
      break;

    case SYS_OPEN :
      //printf ("open call!\n");
      argv[0] = get_argument (sp);
      f->eax = syscall_open ((char *) *(uint32_t *)argv[0]);
      break;

    case SYS_FILESIZE :
      //printf ("filesize call!\n");
      argv[0] = get_argument (sp);
      f->eax = syscall_filesize ((int) *argv[0]);
      break;

    case SYS_READ :
      //printf ("read call!\n");
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      argv[2] = get_argument (sp+2);
      f->eax = (off_t) syscall_read ((int) *argv[0], (void *) *argv[1], (unsigned) *argv[2]);
      break;

    case SYS_WRITE  :
      //printf ("write call!\n");
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      argv[2] = get_argument (sp+2);
      //printf("%d\n", argv[0]);
      //printf("%d\n", argv[1]);
      //printf("%d\n", argv[2]);
      f->eax = (off_t) syscall_write ((int) *argv[0], (void *) *argv[1], (unsigned) *argv[2]);
      break;

    case SYS_SEEK :
      //printf ("seek call!\n");
      argv[0] = get_argument (sp);
      argv[1] = get_argument (sp+1);
      syscall_seek ((int) *argv[0], (unsigned) *argv[1]);
      break;

    case SYS_TELL :
      //printf ("tell call!\n");
      argv[0] = get_argument (sp);
      f->eax = syscall_tell ((int) *argv[0]);
      break;

    case SYS_CLOSE  :
      //printf ("close call!\n");
      argv[0] = get_argument (sp);
      syscall_close ((int) *argv[0]);
      break;

    default :
      break;
      //printf("Unknown System Call");
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
  //printf("in syscall_exec, cmd line is %s\n", cmd_line);
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
syscall_read (int fd, const void *buffer, unsigned size) {
  validate_addr ((void *) buffer);
  validate_addr((void *)(buffer + size));
  if(!is_user_vaddr(buffer + size)) syscall_exit(-1);

  if (fd == 1) syscall_exit (-1);

  int value = -1;
  int i;

  lock_acquire (&file_lock);

  if (fd == 0) {
    //// 여기 고쳐야함
    for (i = 0; i < (int) size; i++) {

      *(uint8_t *)buffer = input_getc();
      buffer++;
      validate_addr ((void *) buffer);
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
syscall_write (int fd, const void *buffer, unsigned size) {
  validate_addr ((void *)buffer);
  validate_addr ((void *)(buffer + size));

  if(!is_user_vaddr(buffer)) syscall_exit(-1);
  if(!is_user_vaddr(buffer + size)) syscall_exit(-1);

  int value = -1;

  if (fd == 0) syscall_exit (-1);

  lock_acquire (&file_lock);

  if (fd == 1) {
    //// 여기 고쳐야함
    putbuf (buffer, size);
    value = size;
  } else {
    /// if no file, exit
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

  return file_seek (f, position);//
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

  if (f == NULL || f_elem == NULL) 
    syscall_exit (-1);
  
  list_remove (&f_elem->elem);
  file_close (f);
  free (f_elem);
}



