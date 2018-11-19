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

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int *args = (int *)f->esp;


  for(int i =0 ; i< 4; i++)
  {
    if(!is_user_vaddr( &args[i]))
    {
      printf("%s: exit(%d)\n", thread_current()->name, -1);
      sema_up(&child_sema);
      thread_exit ();
    }
  }
 
  switch(args[0])
  {
     case SYS_HALT:
      shutdown_power_off();
            printf("SYS_HALT");
            break;
    
   case SYS_EXIT:
	printf("%s: exit(%d)\n", thread_current()->name, args[1]);
  sema_up(&child_sema);
  thread_exit ();
        
        printf("SYS_EXIT");
        break;
   case SYS_EXEC:
        printf("SYS_EXEC");
        break;
   case SYS_WAIT:
        printf("SYS_WAIT");
        break;
   case SYS_CREATE:
        break;
   case SYS_REMOVE:
        printf("SYS_REMOVE");
        break;
   case SYS_OPEN:
        printf("SYS_OPEN");
        break;
   case SYS_FILESIZE:
        printf("SYS_FILESIZE");
        break;
   case SYS_READ:
        printf("SYS_READ");
        break;
   case SYS_WRITE:
        if (args[1] == STDOUT_FILENO)
    {
      putbuf ((void *)(args[2]), (unsigned int)args[3]);
      
      f->eax =  (unsigned int)args[3];
    }
          
        
        break;
   case SYS_SEEK:
        printf("SYS_SEEK");
        break;
   case SYS_TELL:
        printf("SYS_TELL");
        break;
   case SYS_CLOSE:
        printf("SYS_CLOSE");
        break;
  }
}
   