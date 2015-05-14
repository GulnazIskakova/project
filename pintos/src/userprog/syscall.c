#include "filesys/file.h"
#include "filesys/filesys.h"

#include "userprog/syscall.h"
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include <stdio.h>
#include <syscall-nr.h>

#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

#define MAX_ARGS 4

// fd = file descriptor
// fd = integer
//new fns
int UK_pointer (const void* vaddr);
int proc_newfile (struct file *file);
void proc_byefile (int fd);
struct file* getfile (int fd);


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
//  printf ("system call!\n");
//  thread_exit ();

    int args[MAX_ARGS];
    for (int i = 0; i < MAX_ARGS; ++i)
        args[i] = *( (int*) f->esp + i);

    switch (arg[0])
    {
        case SYS_HALT:
        {
            halt();
            break;
        }

        case SYS_EXIT:
        {
            exit(arg[1]);
            break;
        }

        case SYS_EXEC:
        {
            arg[1] = UK_pointer((const void *) arg[1]);
            exec((const char *) arg[1]);
            break;
        }

        case SYS_WAIT:
        {
            wait(arg[1]);
            break;
        }

        case SYS_CREATE:
        {
            arg[1] = UK_pointer((const void *) arg[1]);
            create((const char *)arg[1], (unsigned) arg[2]);
            break;
        }

        case SYS_REMOVE:
        {
            arg[1] = UK_pointer((const void *) arg[1]);
            remove((const char *)arg[1]);
            break;
        }

        case SYS_OPEN:
        {
            arg[1] = UK_pointer((const void *) arg[1]);
            open((const char *) arg[1]);
            break;
        }

        case SYS_FILESIZE:
        {
            filesize(arg[1]);
            break;
        }

        case SYS_READ:
        {
            arg[2] = UK_pointer((const void *) arg[2]);
            read(arg[1], (void *) arg[2], (unsigned) arg[3]);
            break;
        }

        case SYS_WRITE:
        {
            arg[2] = UK_pointer((const void *) arg[2]);
            write(arg[1], (const void *) arg[2], (unsigned) arg[3]);
            break;
        }

        case SYS_SEEK:
        {
            seek(arg[1], (unsigned) arg[2]);
            break;
        }

        case SYS_TELL:
        {
            tell(arg[1]);
            break;
        }

        case SYS_CLOSE:
        {
            close(arg[1]);
            break;
        }
    }
}

void halt (void)
{
    shutdown_power_off();
}

void exit (int status)
{
    printf ("%s: exit(






































}
















































































