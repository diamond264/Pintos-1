#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct thread_id {
	tid_t tid;
	struct process process;
	struct list_elem elem;
}

// struct process {
// 	pid_t pid;
// 	int status;
// 	bool loaded;
// 	bool dead;

// 	pid_t parent;
// 	struct list child_thread;
// 	struct semaphore sema;
// }

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
