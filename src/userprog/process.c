#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "userprog/syscall.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

extern struct lock page_lock;
extern struct lock file_lock;

// parent thread의 chilren list에서 tid를 가지는 element를 꺼내오기 위한 함수
struct child_elem* get_child(struct thread *parent, tid_t child_tid)
{
  struct list_elem *iter;
  struct child_elem *t;

  if (parent == NULL) return NULL;

  if (list_empty (&parent->children)) return NULL;

  for(iter = list_begin(&parent->children); iter != list_end(&parent->children); iter = list_next(iter))
  {
    t = list_entry(iter, struct child_elem, elem);
    if (t == NULL) return NULL;

    if(t->tid == child_tid)
      return t;
  }

  return NULL;
}

// parent thread의 children list에서 child element를 제거한다.
void remove_child(struct child_elem *child)
{
  list_remove(&child->elem);
}

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  struct thread *curr = thread_current();

  sema_init (&curr->sema_start,0);
  sema_init (&curr->sema_exit,0);

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  // File name을 parsing 하기 위해 한 번 더 copy 해준다. (to avoid race condition)
  char *fn_copy2;

  fn_copy2 = palloc_get_page(0);
  if(fn_copy2 == NULL)
    return TID_ERROR;
  strlcpy(fn_copy2, file_name, PGSIZE);

  // args[0]을 가져와 real filename을 parsing한다.
  char *save_ptr;
  char *rf_name = strtok_r(fn_copy2, " ", &save_ptr); // real file name

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (rf_name, PRI_DEFAULT, start_process, fn_copy);

  if (tid == TID_ERROR) {
    palloc_free_page (fn_copy);
    palloc_free_page(fn_copy2);
    free(rf_name);

    return -1;
  }

  // thread_create에서 children list에 넣어준 tid를 꺼내 준다.
  struct child_elem *child = get_child(curr, tid);

  // start_process에서 sema_up을 기다리기 위해 sema_down
  sema_down(&curr->sema_start);
  
  // load가 되지 않았을 때, -1을 return하기 위한 작업
  if(child != NULL && !child->loaded) {
    remove_child(child);
    free (child);
    return -1;
  }

  palloc_free_page(fn_copy2);
  return tid;
}

/* A thread function that loads a user process and makes it start
   running. */
static void
start_process (void *f_name)
{
  char *file_name = f_name;
  struct intr_frame if_;
  bool success;
  struct thread *curr = thread_current();

  hash_init (&curr->spage_table, hash_calc_func, hash_comp_func, NULL);

  // Argument Passing

  // file_name parse with spliting space
  char args[100][100] = {0,}; // arguments를 저장할 array
  char *args_addr[100] = {0,}; // args의 pointer를 저장할 array

  int argc = 0;
  char *token, *save_ptr;
  token = strtok_r(file_name, " ", &save_ptr);

  /* Initialize interrupt frame and load executable. */

  // File Deny Write를 먼저 해준다.
  struct file *userprog = filesys_open(token);
  if(userprog != NULL)
  {
    file_deny_write(userprog);
  }
  else
  {
    palloc_free_page(file_name);
    file_close(userprog);
    if (!list_empty (&curr->parent->sema_start.waiters))
      sema_up(&curr->parent->sema_start);
    syscall_exit(-1);
  }

  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (token, &if_.eip, &if_.esp);

  if(!success)
  {
    palloc_free_page(file_name);
    file_close(userprog);
    if (!list_empty (&curr->parent->sema_start.waiters))
      sema_up(&curr->parent->sema_start);
    syscall_exit(-1);
  }

  struct thread *parent = curr->parent;
  struct child_elem *curr_elem = get_child (parent, curr->tid);
  curr_elem->loaded = success;

  curr->program = userprog;

  // load 끝나면 sema up 해준다.
  if (!list_empty (&parent->sema_start.waiters))
    sema_up(&curr->parent->sema_start);

  for(; token != NULL; token = strtok_r(NULL, " ", &save_ptr))
  {
    int len = strlen(token) + 1;
    if_.esp -= len;
    memcpy(if_.esp, token, len);
    args_addr[argc++] = if_.esp;
  }

  // Offset
  if_.esp -= (uint32_t)if_.esp % 4;

  // args[argc]
  if_.esp -= 4;
  *(int*)if_.esp = 0;

  // args[0~argc-1]
  int i;
  for(i = argc - 1; i >= 0; i--)
  {
    if_.esp -= 4;
    *(void**)if_.esp = args_addr[i];
  }

  // args 주소를 넣어준다.
  if_.esp -= 4;
  *(void**)if_.esp = if_.esp + 4;

  // 그 다음 argc를 넣어준다.
  if_.esp -= 4;
  *(int*)if_.esp = argc;

  // 마지막으로 return address를 넣어준다.
  if_.esp -= 4;
  *(int*)if_.esp = 0;


  /* If load failed, quit. */
  palloc_free_page (file_name);

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.
   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid) 
{
  int status;
  struct thread *t_parent;
  struct child_elem *t_child;

  t_parent = thread_current ();
  t_child = get_child (t_parent, child_tid);
  if (t_child == NULL)
    return -1;

  if (!t_child->terminated)
  {
    t_parent->waiting = child_tid;

    sema_down (&t_parent->sema_exit);
  }

  status = t_child->exit_status;
  remove_child(t_child);
  free(t_child);

  return status;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *curr = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */

  pd = curr->pagedir;
  if (pd != NULL) 
  {
    /* Correct ordering here is crucial.  We must set
       cur->pagedir to NULL before switching page directories,
       so that a timer interrupt can't switch back to the
       process page directory.  We must activate the base page
       directory before destroying the process's page
       directory, or our active page directory will be one
       that's been freed (and cleared). */

    struct thread *t_parent = curr->parent;
    struct child_elem *curr_elem = get_child (t_parent, curr->tid);

    curr_elem->terminated = true;
    curr_elem->exit_status = curr->exit_status;

    if(curr->program != NULL)
    {
      file_close(curr->program);
      curr->program = NULL;
    }

    // EDITED
    printf("%s: exit(%d)\n", curr->name, curr->exit_status);
    t_parent->child_exit_status = curr->exit_status;

    if (curr_elem->tid == t_parent->waiting && !list_empty (&t_parent->sema_exit.waiters))
    {
      sema_up (&t_parent->sema_exit);
    }

    struct list_elem *t_elem;
    struct child_elem *t;

    struct list_elem *f_elem;
    struct file_elem *f;

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
    
    lock_acquire (&page_lock);

    // spage table의 spage들을 모두 free.
    hash_destroy(&curr->spage_table, hash_free_func);

    curr->pagedir = NULL;
    pagedir_activate (NULL);
    pagedir_destroy (pd);

    lock_release (&page_lock);
  }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in ////printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
/*
  load 함수에서 pagedir ptr을 새로 할당받아온다.
*/
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  /* Open executable file. */
  file = filesys_open (file_name);
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:
        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.
        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.
   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);

  lock_acquire(&page_lock);
  while (read_bytes > 0 || zero_bytes > 0) 
  {
    /* Do calculate how to fill this page.
       We will read PAGE_READ_BYTES bytes from FILE
       and zero the final PAGE_ZERO_BYTES bytes. */
    size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    struct spage *spe;

    if(PGSIZE == page_zero_bytes)
    {
      spe = spage_create(upage, LAZY, writable);
      spe->is_zero = true;
    }
    else if(PGSIZE == page_read_bytes)
    {
      spe = spage_create(upage, LAZY, writable);
      spe->offset = file_tell(file); // 현재 파일 포인터 저장
      file_seek(file, file_tell(file) + PGSIZE);
    }
    else
    {
      spe = spage_create(upage, PAGE, writable);
      struct frame *f = frame_allocate(spe, PAL_USER);
      uint8_t *kpage = f->addr;

      /* Get a page of memory. */
      //uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
      {
        ASSERT(0);
        return false;
      }

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
      {
        frame_free(f);
        lock_release(&page_lock);
        //palloc_free_page (kpage);
        return false; 
      }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
      {
        frame_free(f);
        //palloc_free_page (kpage);
        lock_release(&page_lock);
        return false; 
      }
    }
    /* Advance. */
    read_bytes -= page_read_bytes;
    zero_bytes -= page_zero_bytes;
    upage += PGSIZE;
  }
  lock_release(&page_lock);
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp) 
{
  uint8_t *kpage;
  bool success = false;
  void *upage = ((uint8_t *) PHYS_BASE) - PGSIZE;
  if (upage == 0) ASSERT(0);

  lock_acquire(&page_lock);
  struct spage *spe = spage_create(upage, PAGE, true);
  struct frame *f = frame_allocate(spe, PAL_USER | PAL_ZERO);

  kpage = f->addr;

  //kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
  {
    success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
    if (success)
    {
      *esp = PHYS_BASE;
    }
    else
      //palloc_free_page (kpage);
      frame_free(f);
  }
  lock_release(&page_lock);

  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

void* validate_addr (void *addr)
{
  struct thread *curr = thread_current();

  if (addr == NULL || is_kernel_vaddr(addr) || pagedir_get_page (curr->pagedir, addr) == NULL)
  {
    syscall_exit(-1);
    return NULL;
  }
  else 
    return addr;
}

void validate_addr_syscall (struct intr_frame *f, void *fault_addr)
{
  struct thread * curr = thread_current ();
  void *rounded_addr = pg_round_down (fault_addr);

  if (fault_addr == NULL
    || !is_user_vaddr(fault_addr))
  {
    if(lock_held_by_current_thread(&file_lock))
      lock_release(&file_lock);
    syscall_exit (-1);
  }

  if (pagedir_get_page (curr->pagedir, fault_addr) == NULL)
  {
    if (fault_addr >= (f->esp - 32))
      stack_growth (rounded_addr);
  }
}