#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"
//bool setup_stack(void **esp, const char* file_name, char** save_ptr*);
tid_t process_execute (const char *file_name);
int process_wait (tid_t child_tid UNUSED);
void process_exit(void);
void process_activate (void);

#endif /* userprog/process.h */
