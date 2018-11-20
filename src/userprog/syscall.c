#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"

static int desc_number = 1;
static void syscall_handler(struct intr_frame *);
struct lock files_lock;
struct list files_list;

struct file_info
{
  struct list_elem files_list;
  struct file *file_ptr;
  int file_num;
  struct thread *held_by;
};

static struct file_info *get_fd(int file_desc, bool remove)
{
  struct list_elem *e;

  for (e = list_begin(&files_list); e != list_end(&files_list);
       e = list_next(e))
  {
    struct file_info *f = list_entry(e, struct file_info, files_list);
    if (f->file_num == file_desc && f->held_by->tid == thread_current()->tid)
    {
      if (remove)
        list_remove(e);
      return f;
    }
  }
  return NULL;
}

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&files_lock);
  list_init(&files_list);
}
void invalid_panic(void *ptr)
{
  if (ptr == NULL || !is_user_vaddr(ptr) || (pagedir_get_page(thread_current()->pagedir, ptr)) == NULL)
  {
    printf("%s: exit(%d)\n", thread_current()->name, -1);
    sema_up(&child_sema);
    thread_exit();
  }
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  uint32_t *args = f->esp;

  invalid_panic(f->esp);

  invalid_panic(args);
  invalid_panic(args + 3);

  switch (args[0])
  {
  case SYS_HALT:
    shutdown_power_off();
    printf("SYS_HALT");
    break;

  case SYS_EXIT:
    printf("%s: exit(%d)\n", thread_current()->name, args[1]);
    sema_up(&child_sema);
    thread_exit();

    printf("SYS_EXIT");
    break;

  case SYS_READ:

    invalid_panic((void *)args[2]);
    invalid_panic(((void *)args[2]) + args[3]);
    lock_acquire(&files_lock);
    if (args[1] == STDIN_FILENO)
      for (int offset = 0; offset < args[3]; ++offset)
        *(uint8_t *)((char *)args[2] + offset) = input_getc();
    f->eax = args[3];
    if (args[1] == STDOUT_FILENO)
      f->eax = -1;
    else
    {
      struct file_info *fd = get_fd(args[1],false);
      if(fd != NULL)
      {
          f->eax = file_read(fd->file_ptr,(void *)args[2],args[3]);
      }
      else
        f->eax = -1;
    }
    lock_release(&files_lock);
    break;
  case SYS_WRITE:

    invalid_panic((void *)args[2]);
    invalid_panic(((void *)args[2]) + args[3]);
    lock_acquire(&files_lock);
    if (args[1] == STDOUT_FILENO)
    {
      putbuf((void *)(args[2]), (unsigned int)args[3]);

      f->eax = (unsigned int)args[3];
    }
    else if (args[1] == STDIN_FILENO)
      f->eax = -1;
    else
    {
      struct file_info *file_ref = get_fd(args[1],false);
      if(file_ref != NULL)
        f->eax = file_write(file_ref->file_ptr,(void *)args[2],args[3]);
      else
        f->eax = 0;
    }
    lock_release(&files_lock);
    break;

  case SYS_CREATE:
    invalid_panic((void *)args[1]);
    lock_acquire(&files_lock);
    f->eax = filesys_create((char *)args[1], args[2]);
    lock_release(&files_lock);
    break;
  case SYS_OPEN:
    invalid_panic((void *)args[1]);
    
    struct file *fd = filesys_open((char *)args[1]);
    if (fd == NULL)
      f->eax = -1;
    else
    {
      lock_acquire(&files_lock);
      f->eax = ++desc_number;
      struct file_info *file_node = malloc(sizeof(struct file_info));
      file_node->file_num = f->eax;
      file_node->held_by = thread_current();
      file_node->file_ptr = fd;
      list_push_back(&files_list,&file_node->files_list);
      lock_release(&files_lock);
    }
    break;
  case SYS_CLOSE:
    lock_acquire(&files_lock);
    struct file_info *file_ref = get_fd(args[1],true);
    if(file_ref != NULL)
        file_close(file_ref->file_ptr);
    lock_release(&files_lock);
        break;
  case SYS_FILESIZE: 
    lock_acquire(&files_lock);
    file_ref = get_fd(args[1],false);
    f->eax = file_length(file_ref->file_ptr);
    lock_release(&files_lock);

      break;
  case SYS_SEEK:
    lock_acquire(&files_lock);
      file_ref  = get_fd(args[1],false);
      if(file_ref != NULL)
        file_seek(file_ref->file_ptr,args[2]);

      lock_release(&files_lock);
    break;

  case SYS_TELL:
    lock_acquire(&files_lock);
      file_ref  = get_fd(args[1],false);
      if(file_ref != NULL)
        f->eax = file_tell(file_ref->file_ptr);

      lock_release(&files_lock);

  break;

  case SYS_EXEC:

      f->eax = process_execute((char *)args[1]);
      break;
  case SYS_WAIT:
       process_wait(args[1]);
      break;
  case SYS_REMOVE:

  
  default:
    printf("%s: exit(%d)\n", thread_current()->name, -1);
    sema_up(&child_sema);
    thread_exit();
  }
}
