#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include <filesys/filesys.h>
#include "filesys/off_t.h"

static void syscall_handler (struct intr_frame *);
void * check_address(void * address);
void get_arguments (struct intr_frame * f, int * args, int count);

void halt();
void exit(int status);
tid_t exec(const char * cmd_line);
int wait(tid_t tid);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&check_file);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * pointer = f->esp;
  check_address((void *)pointer);
  int syscall_case = *pointer;
  int args[3];

  switch (syscall_case) {
    case 0:
      halt();
      break;
    case 1:
      get_arguments(f, args, 1);
      thread_current()->exit_status = args[0];
      exit(thread_current()->exit_status);
      break;
    case 2:
      get_arguments(f, args, 1);
      f->eax = exec((const char *)args[0]);
      break;
    case 3:
      get_arguments(f, args, 1);
      f->eax = wait((const char *)args[0]);
      break;
    case 4:
      get_arguments(f, args, 2);
      break;
    case 5:
      get_arguments(f, args, 1);
      break;
    case 6:
      get_arguments(f, args, 1);
      break;
    case 7:
      get_arguments(f, args, 1);
      break;
    case 8:
      get_arguments(f, args, 3);
      break;
    case 9:
      get_arguments(f, args, 3);
      break;
    case 10:
      get_arguments(f, args, 2);
      break;
    case 11:
      get_arguments(f, args, 1);
      break;
    case 12:
      get_arguments(f, args, 1);
      break;
  }
}

void * check_address(void * address) {
	if (is_user_vaddr(address)) {
		void * pointer = pagedir_get_page(thread_current()->pagedir, address);
	  if (pointer) {
		  return pointer;
    }
	}
	exit(-1);
}

void get_arguments (struct intr_frame * f, int * args, int count) {
  for (int i = 0; i < count; i++) {
    int * pointer = (int *) f->esp + i + 1;
    check_address((const void *) pointer);
    args[i] = * pointer;
  }
}

void halt() {
  shutdown_power_off();
}

void exit(int status) {
  printf ("%s: exit(%d)\n", thread_name(), status);
  for (int i = 3; i < 131; i++) {
    if (thread_current()->fd[i] != NULL) {
      close(i);
    }   
  }
  thread_current()->exit_status = status;
  thread_exit();
}

tid_t exec(const char * cmd_line) {
  int child_pid = process_execute(cmd_line);
  return child_pid;
}

int wait(tid_t tid) {
  int return_tid = process_wait(tid);
  return return_tid;
}