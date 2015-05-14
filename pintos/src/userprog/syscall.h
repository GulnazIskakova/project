#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

#define CLOSE_ALL -1
#define ERROR -1

#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL 2

struct child_process {
    int pid;
    int load;
    bool wait;
    bool exit;
    int status;
    struct lock wait_lock;
    struct list_elem elem;
};

struct child_process* newchild (int pid);
struct child_process* getchild (int pid);
void byechild (struct child_process *cp);
void byechildren (void);

void byefile (int fd);

void syscall_init (void);

#endif /* userprog/syscall.h */
