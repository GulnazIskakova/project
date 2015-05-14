#include "filesys/file.h"
#include "filesys/filesys.h"

#include "threads/malloc.h"
#include "threads/synch.h"

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

//#define ERROR -1
#define MAX_ARGS 4

// new vars/structs
struct lock filesys_lock;

struct process_file {
    struct file *file;
    int fd;
    struct list_elem elem;
};

// fd = file descriptor
// fd = integer
//new fns
int UK_pointer (const void *vaddr); // user to kernel
int newfile (struct file *file);
void byefile (int fd);

static void syscall_handler (struct intr_frame *);


void
syscall_init (void) 
{
    lock_init(&filesys_lock);
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
            f->eax = exec((const char *) arg[1]);
            break;
        }

        case SYS_WAIT:
        {
            f->eax = wait(arg[1]);
            break;
        }

        case SYS_CREATE:
        {
            arg[1] = UK_pointer((const void *) arg[1]);
            f->eax = create((const char *)arg[1], (unsigned) arg[2]);
            break;
        }

        case SYS_REMOVE:
        {
            arg[1] = UK_pointer((const void *) arg[1]);
            f->eax = remove((const char *)arg[1]);
            break;
        }

        case SYS_OPEN:
        {
            arg[1] = UK_pointer((const void *) arg[1]);
            f->eax = open((const char *) arg[1]);
            break;
        }

        case SYS_FILESIZE:
        {
            f->eax = filesize(arg[1]);
            break;
        }

        case SYS_READ:
        {
            arg[2] = UK_pointer((const void *) arg[2]);
            f->eax = read(arg[1], (void *) arg[2], (unsigned) arg[3]);
            break;
        }

        case SYS_WRITE:
        {
            arg[2] = UK_pointer((const void *) arg[2]);
            f->eax = write(arg[1], (const void *) arg[2], (unsigned) arg[3]);
            break;
        }

        case SYS_SEEK:
        {
            seek(arg[1], (unsigned) arg[2]);
            break;
        }

        case SYS_TELL:
        {
            f->eax = tell(arg[1]);
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
    struct thread *cur = thread_current();
    bool exists = thread_alive(cur->parent);
    if (exists)
    {
        if (cur->cp->wait)
            cur->cp->status = status;

    }

    printf ("%s: exit(%d)\n", cur()->name, status);
    thread_exit();
}

pid_t exec (const char *cmd_line)
{
    pid_t pid = process_execute(cmd_line);

    struct child_process *cp = getchild(pid);
    if (!cp)
        return ERROR;
    
    while (cp->load == NOT_LOADED)
    {
        // block thread
        barrier();
    }
    
    if(cp->load == LOAD_FAIL)
        return ERROR;

    return pid;
}

int wait (pid_t pid)
{
    return process_wait(pid);
}

bool create (const char *file, unsigned init_size)
{
    lock_acquire(&filesys_lock);
    bool success = filesys_create(file, init_size);
    lock_release(&filesys_lock);
    return success;
}

bool remove (const char *file)
{
    lock_acquire(&filesys_lock);    
    bool success = filesys_remove(file);
    lock_release(&filesys_lock);
    return success;    
}

int open (const char *file)
{
    lock_acquire(&filesys_lock);
    
    struct file *f = filesys_open(file);
    if (!f)
    {
        lock_release(&filesys_lock);
        return ERROR;
    }

    int fd = newfile(f);
    lock_release(&filesys_lock);

    return fd;
}

int filesize(int fd)
{
    lock_acquire(&filesys_lock);
    struct file *f = getfile(fd);

    if (!f)
    {
        lock_release(&filesys_lock);
        return ERROR;
    }
 
    int size = file_length(f);
    lock_release(&filesys_lock);
    return size;
}

int read (int fd, void *buffer, unsigned size)
{
    if (fd == STDIN_FILENO)
    {
        uint8_t* local_buffer = (uint8_t *) buffer;
        for (unsigned i = 0; i < size; ++i)
           local_buffer[i] = input_getc();
        
        return size;
    }
    
    lock_acquire(&filesys_lock);

    struct file *f = getfile(fd);

    if(!f)
    {
        lock_release(&filesys_lock);
        return ERROR;
    }

    int bytes = file_read(f, buffer, size);

    lock_release(&filesys_lock);

    return bytes;
}

int write (int fd, const void *buffer, unsigned size)
{
    if (fd == STDOUT_FILENO)
    {
        putbuf(buffer, size);
        return size;
    }

    lock_acquire(&filesys_lock);
    
    struct file *f = getfile(fd);

    if (!f)
    {
        lock_release(&filesys_lock);
        return ERROR;
    }

    int bytes = file_write(f, buffer, size);

    lock_release(&filesys_lock);

    return bytes;
}

void seek (int fd, unsigned position)
{
    lock_acquire(&filesys_lock);

    struct file *f = getfile(fd);
    
    if(!f)
    {
        lock_release(&filesys_lock);
        return;
    }

    file_seek(f, position);

    lock_release(&filesys_lock);
}

unsigned tell (int fd)
{
    lock_acquire(&filesys_lock);

    struct file *f = getfile(fd);

    if (!f)
    {
        lock_release(&filesys_lock);
        return ERROR;
    }

   
    off_t offet = file_tell(f);

    lock_release(&filesys_lock);

    return offset;
}

void close (int fd)
{
    lock_acquire(&filesys_lock);
    byefile(fd);
    lock_release(&filesys_lock);
}

int UK_pointer(const void *vaddr)
{
    if (!is_user_addr(vaddr))
    {
        thread_exit();
        return 0;
    }

    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (!ptr)
    {
        thread_exit();
        return 0;
    }

    return (int) ptr;
}

int newfile (struct file *f)
{
   struct process_file *pf = malloc(sizeof(struct process_file));
   pf->file = f;
   pf->fd = thread_current()->fd;
   thread_current()->fd++;
   list_push_back(&thread_current()->files, &pf->elem);
   return pf->fd;
}

struct file* getfile (int fd)
{
    struct thread *t = thread_current();
    struct list_elem *e;

    for (e = list_begin (&t->files); e != list_end (&t->file_list); e = list_next(e))
    {
        struct process_file *pf = list_entry (e, struct process_file, elem);
        if (fd == pf->fd)
            return pf->file;
    }

    return NULL;
}

void byefile (int fd)
{
    struct thread *t = thread_current();
    struct list_elem *next, *e = list_begin(&t->file_list);

    while (e != list_end (&t->files))
    {
        next = list_next(e);
        struct process_file *pf = list_entry (e, struct process_file, elem);
        if (fd == pf->fd || fd == CLOSE_ALL)
        {
            file_close(pf->file);
            list_remove(&pf->elem);
            free(pf);
            if (fd != CLOSE_ALL)
                return;

        }

        e = next;
    }
}

struct child_process* newchild (int pid)
{
    struct child_process *cp = malloc(sizeof(struct child_process));
    cp->pid = pid;
    cp->load = NOT_LOADED;
    cp->wait = false;
    cp->exit = false;
    lock_init(&cp->wait_lock);
    list_push_back(&thread_current()->child_list, &cp->elem);
    return cp;
}

struct child_process* getchild (int pid)
{
    struct thread *t = thread_current();
    struct list_elem *e;

    for (e = list_begin (&t->child_list); e != list_end (&t->child_list); e = list_next(e))
    {
        struct child_process *cp = list_entry (e, struct child_process, elem);
        if (pid == cp->pid)
            return cp;
    }

    return NULL;
}

void byechild (struct child_process *cp)
{
    list_remove(&cp->elem);
    free(cp);
}

void byechildren (void)
{
    struct thread *t = thread_current();
    struct list_elem *next, *e = list_begin(&t->child_list);

    while (e != list_end (&t->child_list))
    {
        next = list_next(e);
        struct child_process *cp = list_entry (e, struct child_process, elem);

        list_remove(&cp->elem);
        free(cp);
        e = next;
    }
}






















































    
















































































