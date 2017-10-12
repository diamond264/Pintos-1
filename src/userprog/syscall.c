#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
struct lock file_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&file_lock);
}

static void
syscall_handler (struct intr_frame *f) 
{
  printf ("system call!\n");
  uint32_t *argv[5];

  switch (f->esp) {
  	case SYS_HALT	:
      halt ();
  		break;

  	case SYS_EXIT	:
      argv[0] = f->esp + 1;
      exit ((int) *argv[0]);
  		break;

  	case SYS_EXEC	:
      argv[0] = f->esp + 1;
      f->eax = exec ((char *) *argv[0]);
  		break;

  	case SYS_WAIT	:
      argv[0] = f->esp + 1;
      f->eax = wait ((pid_t) *argv[0]);
  		break;

  	case SYS_CREATE	:
  		break;
  	case SYS_REMOVE	:
  		break;
  	case SYS_OPEN	:
  		break;
  	case SYS_FILESIZE	:
  		break;
  	case SYS_READ	:
  		break;
  	case SYS_WRITE	:
  		break;
  	case SYS_SEEK	:
  		break;
  	case SYS_TELL	:
  		break;
    case SYS_CLOSE	:
    	break;

    default :
    	thread_exit ();
  }
}

void
halt (void) {
  power_off ();
}

void
exit (int status) {
  struct thread *t;
  t = thread_current ();
  t->exit_status = status;
  thread_exit ();
}

pid_t
exec (const char *cmd_line) {
  return (pid_t) process_execute (cmd_line);
}

int
wait (pid_t pid) {
  return process_wait (pid);
}


int
read (int fd, const void *buffer, unsigned size) {
  int value = -1;

  if (fd == 1) exit (-1);

  lock_acquire (&file_lock);

  if (fd == 0) {
    //// 여기 고쳐야함
    for (i = 0; i < (int) size; i++) {
      *(uint8_t *)buffer = input_getc();
      buffer++;
    }
    value = size;
  } else {
    /// if no file, exit
    value  = file_read(get_file(fd), buffer, (off_t) size);
  }

  lock_release(&file_lock);
  return value;
}

int
write (int fd, const void *buffer, unsigned size) {
  int value = -1;

  if (fd == 0) exit (-1);

  lock_acquire (&file_lock);

  if (fd == 1) {
    //// 여기 고쳐야함
    putbuf (buffer, size);
    value = size;
  } else {
    /// if no file, exit
    value  = file_write(get_file(fd), buffer, (off_t) size);
  }

  lock_release(&file_lock);
  return value;
}

struct file*
get_file (int fd) {
  struct thread curr = thread_current ();
  struct list_elem *i;

  for (i=list_begin (&curr->files); i!=list_end (&curr->files); i=list_next(i)) {
    struct file_elem file_i = list_entry (i, struct file_elem, elem)
    if (file_i->fd == fd) {
      return file_i->file;
    }
  }
}



