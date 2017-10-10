#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  printf ("system call!\n");
  uint32_t *argv;

  switch (f->esp) {
  	case SYS_HALT	:
      halt ();
  		break;

  	case SYS_EXIT	:
      argv = f->esp + 1;
      exit ((int) *argv);
  		break;

  	case SYS_EXEC	:
      argv = f->esp + 1;
      f->eax = exec ((char *) *argv);
  		break;

  	case SYS_WAIT	:
      argv = f->esp + 1;
      f->eax = wait ((pid_t) *argv);
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
  struct process *p;
  p = thread_current ()->process;
  p->status = status;
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





