#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct thread;
void syscall_init (void);
void close_all_files (struct thread * t);

#endif /* userprog/syscall.h */
