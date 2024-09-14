#include "klib.h"
#include "cte.h"
#include "sysnum.h"
#include "vme.h"
#include "serial.h"
#include "loader.h"
#include "proc.h"
#include "timer.h"
#include "file.h"

typedef int (*syshandle_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

extern void *syscall_handle[NR_SYS];

void do_syscall(Context *ctx) {
  // TODO: WEEK2-interrupt call specific syscall handle and set ctx register
  int sysnum = 0;
  uint32_t arg1 = 0;
  uint32_t arg2 = 0;
  uint32_t arg3 = 0;
  uint32_t arg4 = 0;
  uint32_t arg5 = 0;
  int res;
  if (sysnum < 0 || sysnum >= NR_SYS) {
    res = -1;
  } else {
    res = ((syshandle_t)(syscall_handle[sysnum]))(arg1, arg2, arg3, arg4, arg5);
  }
  ctx->eax = res;
}

int sys_write(int fd, const void *buf, size_t count) {
  // TODO: rewrite me at Lab3-1
  return serial_write(buf, count);
}

int sys_read(int fd, void *buf, size_t count) {
  // TODO: rewrite me at Lab3-1
  return serial_read(buf, count);
}

int sys_brk(void *addr) {
  // TODO: WEEK3-virtual-memory
  // proc_t * proc = proc_curr();// uncomment me in WEEK3-virtual-memory
  size_t brk = 0; // rewrite me
  size_t new_brk = 0; // rewrite me
  if (brk == 0) {
    // proc_curr()->brk = new_brk; // uncomment me in WEEK3-virtual-memory
  } else if (new_brk > brk) {
    TODO();
  } else if (new_brk < brk) {
    // can just do nothing
    // recover memory, Lab 1 extend
  }
  return 0;
}

void sys_sleep(int ticks) {
  // TODO(); // WEEK2-interrupt
  uint32_t beg_tick = get_tick();
  while(get_tick() - beg_tick <= ticks){
    sti(); hlt(); cli(); // chage to me in WEEK2-interrupt
    // proc_yield(); // change to me in WEEK4-process-api
    // thread_yield();
  }
  return;
}

int sys_exec(const char *path, char *const argv[]) {
  // TODO(); // WEEK2-interrupt, WEEK3-virtual-memory
  // DEFAULT
  printf("sys_exec is not implemented yet.");
  while(1);
}

int sys_getpid() {
  TODO(); // WEEK3-virtual-memory
}

int sys_gettid() {
  TODO(); // Lab2-1
}

void sys_yield() {
  proc_yield();
}

int sys_fork() {
  TODO(); // WEEK4-process-api
}

void sys_exit(int status) {
  TODO();
}

void sys_exit_group(int status) {
  TODO();
  // WEEK4 process api
}

int sys_wait(int *status) {
  TODO(); // WEEK4 process api
}

int sys_sem_open(int value) {
  TODO(); // WEEK5-semaphore
}

int sys_sem_p(int sem_id) {
  TODO(); // WEEK5-semaphore
}

int sys_sem_v(int sem_id) {
  TODO(); // WEEK5-semaphore
}

int sys_sem_close(int sem_id) {
  TODO(); // WEEK5-semaphore
}

int sys_open(const char *path, int mode) {
  TODO(); // Lab3-1
}

int sys_close(int fd) {
  TODO(); // Lab3-1
}

int sys_dup(int fd) {
  TODO(); // Lab3-1
}

uint32_t sys_lseek(int fd, uint32_t off, int whence) {
  TODO(); // Lab3-1
}

int sys_fstat(int fd, struct stat *st) {
  TODO(); // Lab3-1
}

int sys_chdir(const char *path) {
  TODO(); // Lab3-2
}

int sys_unlink(const char *path) {
  return iremove(path);
}

// optional syscall

void *sys_mmap() {
  TODO();
}

void sys_munmap(void *addr) {
  TODO();
}

int sys_clone(int (*entry)(void*), void *stack, void *arg, void (*ret_entry)(void)){
  TODO();
}

int sys_join(int tid, void **retval) {
  TODO();
}

int sys_detach(int tid) {
  TODO();
}

int sys_kill(int pid) {
  TODO();
}

int sys_cv_open() {
  TODO();
}

int sys_cv_wait(int cv_id, int sem_id) {
  TODO();
}

int sys_cv_sig(int cv_id) {
  TODO();
}

int sys_cv_sigall(int cv_id) {
  TODO();
}

int sys_cv_close(int cv_id) {
  TODO();
}

int sys_pipe(int fd[2]) {
  TODO();
}

int sys_mkfifo(const char *path, int mode){
  TODO();
}

int sys_link(const char *oldpath, const char *newpath) {
  TODO();
}

int sys_symlink(const char *oldpath, const char *newpath) {
  TODO();
}

void *syscall_handle[NR_SYS] = {
  [SYS_write] = sys_write,
  [SYS_read] = sys_read,
  [SYS_brk] = sys_brk,
  [SYS_sleep] = sys_sleep,
  [SYS_exec] = sys_exec,
  [SYS_getpid] = sys_getpid,
  [SYS_gettid] = sys_gettid,
  [SYS_yield] = sys_yield,
  [SYS_fork] = sys_fork,
  [SYS_exit] = sys_exit,
  [SYS_exit_group] = sys_exit_group,
  [SYS_wait] = sys_wait,
  [SYS_sem_open] = sys_sem_open,
  [SYS_sem_p] = sys_sem_p,
  [SYS_sem_v] = sys_sem_v,
  [SYS_sem_close] = sys_sem_close,
  [SYS_open] = sys_open,
  [SYS_close] = sys_close,
  [SYS_dup] = sys_dup,
  [SYS_lseek] = sys_lseek,
  [SYS_fstat] = sys_fstat,
  [SYS_chdir] = sys_chdir,
  [SYS_unlink] = sys_unlink,
  [SYS_mmap] = sys_mmap,
  [SYS_munmap] = sys_munmap,
  [SYS_clone] = sys_clone,
  [SYS_join] = sys_join,
  [SYS_detach] = sys_detach,
  [SYS_kill] = sys_kill,
  [SYS_cv_open] = sys_cv_open,
  [SYS_cv_wait] = sys_cv_wait,
  [SYS_cv_sig] = sys_cv_sig,
  [SYS_cv_sigall] = sys_cv_sigall,
  [SYS_cv_close] = sys_cv_close,
  [SYS_pipe] = sys_pipe,
  [SYS_mkfifo] = sys_mkfifo,
  [SYS_link] = sys_link,
  [SYS_symlink] = sys_symlink,
  // [SYS_spinlock_open] = sys_spinlock_open,
  // [SYS_spinlock_acquire] = sys_spinlock_acquire,
  // [SYS_spinlock_release] = sys_spinlock_release,
  // [SYS_spinlock_close] = sys_spinlock_close,
};
