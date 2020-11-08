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

void halt();
void exit(int status);
tid_t exec(const char * cmd_line);
int wait(tid_t tid);
bool create(const char * file, unsigned initial_size);
bool remove(const char * file);
int open(const char * file);
int filesize(int fd);
int read(int fd, void * buffer, unsigned size);
int write(int fd, const void * buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * pointer = f->esp;
  check_address((void *)pointer);
  int arguments[3];
  int syscall_case = *pointer;

  switch (syscall_case) {
    case 0:
      halt();
      break;
    case 1:
      get_arguments(f, arguments, 1);
      thread_current()->exit_status = arguments[0];
      exit(thread_current()->exit_status);
      break;
    case 2:
      get_arguments(f, arguments, 1);
      f->eax = exec(arguments[0]);
      break;
    case 3:
      get_arguments(f, arguments, 1);
      f->eax = wait(arguments[0]);
      break;
    case 4:
      get_arguments(f, arguments, 2);
      f->eax = create(arguments[0], arguments[1]);
      break;
    case 5:
      get_arguments(f, arguments, 1);
      f->eax = remove(arguments[0]);
      break;
    case 6:
      get_arguments(f, arguments, 1);
      f->eax = open(arguments[0]);
      break;
    case 7:
      get_arguments(f, arguments, 1);
      f->eax = filesize(arguments[0]);
      break;
    case 8:
      get_arguments(f, arguments, 3);
      check_buffer(arguments[1], arguments[2]);
      f->eax = read(arguments[0], arguments[1], arguments[2]);
      break;
    case 9:
      get_arguments(f, arguments, 3);
      check_buffer(arguments[1], arguments[2]);
      f->eax = write(arguments[0], arguments[1], arguments[2]);
      break;
    case 10:
      get_arguments(f, arguments, 2);
      seek(arguments[0], arguments[1]);
      break;
    case 11:
      get_arguments(f, arguments, 1);
      f->eax = tell(arguments[0]);
      break;
    case 12:
      get_arguments(f, arguments, 1);
      close(arguments[0]);
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

void check_buffer(const void * buffer, int size) {
  for (int i = 0; i < size; i++) {
    check_address(buffer);
    buffer++;
  }
}

void get_arguments(struct intr_frame * f, int * args, int count) {
  for (int i = 0; i < count; i++) {
    int * pointer = (int *) f->esp + i + 1;
    check_address(pointer);
    args[i] = *pointer;
  }
}

void halt() {
  shutdown_power_off();
}

void exit(int status) {
  printf ("%s: exit(%d)\n", thread_name(), status);
  struct thread * current_thread = thread_current();
  close_all_files(current_thread);
  current_thread->exit_status = status;
  thread_exit();
}

tid_t exec(const char * cmd_line) {
  return process_execute(cmd_line);
}

int wait(tid_t tid) {
  return process_wait(tid);
}

bool create(const char * file, unsigned initial_size) {
  if (!file) {
    exit(-1);
  }
  return filesys_create(file, initial_size);
}

bool remove(const char * file) {
  if (!file) {
    exit(-1);
  }
  return filesys_remove(file);
}

int open(const char * file) {
  if (!file) {
    exit(-1);
  }
  lock_acquire(&check_file);
  struct thread * current_thread = thread_current();
  int result = -1;
  struct file* fp = filesys_open(file);
  if (!fp) {
    return result; 
  } else {
    for (int i = 3; i < 131; i++) {
      if (!current_thread->fd[i]) {
        current_thread->fd[i] = fp;
        result = i;
        break;
      }
    }
  }
  return result;
}

int filesize(int fd) {
  struct thread * current_thread = thread_current();
  if (!current_thread->fd[fd]) {
    exit(-1);
  }
  return file_length(current_thread->fd[fd]);
}

int read(int fd, void * buffer, unsigned size) {
  int result = -1;
  if (fd == 0) {
    for (int i = 0; i < size; i++) {
      if (((char *)buffer)[i] == '\0') {
        result = i;
        break;
      }
    }
  } else if (fd > 2) {
    struct thread * current_thread = thread_current();
    if (!current_thread->fd[fd]) {
      exit(-1);
    }
    result = file_read(current_thread->fd[fd], buffer, size);
  }
  return result;
}

int write(int fd, const void * buffer, unsigned size) {
  struct thread * current_thread = thread_current();
  int result = -1;
  if (fd == 1) {
    putbuf(buffer, size);
    result = size;
  } else if (fd > 2) {
    if (!current_thread->fd[fd]) {
      exit(-1);
    }
    result = file_write(current_thread->fd[fd], buffer, size);
  }
  return result;
}

void seek(int fd, unsigned position) {
  struct thread * current_thread = thread_current();
  if (!current_thread->fd[fd]) {
    exit(-1);
  }
  file_seek(current_thread->fd[fd], position);
}

unsigned tell(int fd) {
  struct thread * current_thread = thread_current();
  if (!current_thread->fd[fd]) {
    exit(-1);
  }
  return file_tell(current_thread->fd[fd]);
}

void close(int fd) {
  struct thread * current_thread = thread_current();
  if (!current_thread->fd[fd]) {
    exit(-1);
  }
  file_close(current_thread->fd[fd]);
  current_thread->fd[fd] = NULL;
}

void close_all_files(struct thread * temp) {
  for (int i = 3; i < 131; i++) {
    if (temp->fd[i] != NULL) {
      file_close(temp->fd[i]);
      temp->fd[i] = NULL;
    }
  }
}