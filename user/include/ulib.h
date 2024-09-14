#ifndef __ULIB_H__
#define __ULIB_H__

#include "lib.h"

// compulsory syscall
int write(int fd, const void *buf, size_t count);
int read(int fd, void *buf, size_t count);
int brk(void *addr);
void sleep(int ticks);
int exec(const char *path, char *const argv[]);
int getpid();
int gettid();
void yield();
int fork();
void exit(int status) __attribute__((noreturn));
int wait(int *status);
int sem_open(int value);
int sem_p(int sem_id);
int sem_v(int sem_id);
int sem_close(int sem_id);
int open(const char *path, int mode);
int close(int fd);
int dup(int fd);
uint32_t lseek(int fd, uint32_t off, int whence);
int fstat(int fd, struct stat *st);
int chdir(const char *path);
int unlink(const char *path);

#define P sem_p
#define V sem_v

// optional syscall
void *mmap();
void munmap(void *addr);
int clone(int (*entry)(void*), void *stack, void *arg);
int join(int tid, void **retval);
int detach(int tid);
void thread_exit(int status) __attribute__((noreturn));
int kill(int pid);
int cv_open();
int cv_wait(int cv_id, int sem_id);
int cv_sig(int cv_id);
int cv_sigall(int cv_id);
int cv_close(int cv_id);
// int spinlock_open();
// int spinlock_acquire(int lock_id);
// int spinlock_release(int lock_id);
// int spinlock_close(int lock_id);
int pipe(int fd[2]);
int mkfifo(const char *path, int mode);
int link(const char *oldpath, const char *newpath);
int symlink(const char *oldpath, const char *newpath);

// stdio
void putstr(const char *str);
int printf(const char *format, ...);
int fprintf(int fd, const char *format, ...);
char getchar();
char *getline(char *buf, size_t size);
int scanf(const char *format, ...);

// stdlib
void *sbrk(int increment);
void *malloc(size_t size);
void free(void *ptr);

// assert
int abort(const char *file, int line, const char *info) __attribute__((noreturn));

#define STD_IN  0
#define STD_OUT 1
#define STD_ERR 2

#define MAX_NAME 27

struct dirent {
  size_t node;
  char name[MAX_NAME + 1];
};

#endif
