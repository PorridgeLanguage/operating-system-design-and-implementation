#ifndef __SYSNUM_H__
#define __SYSNUM_H__

#define SYS_write      0
#define SYS_read       1
#define SYS_brk        2
#define SYS_sleep      3
#define SYS_exec       4
#define SYS_getpid     5
#define SYS_gettid     6
#define SYS_yield      7
#define SYS_fork       8
#define SYS_exit       9
#define SYS_wait       10
#define SYS_sem_open   11
#define SYS_sem_p      12
#define SYS_sem_v      13
#define SYS_sem_close  14
#define SYS_open       15
#define SYS_close      16
#define SYS_dup        17
#define SYS_lseek      18
#define SYS_fstat      19
#define SYS_chdir      20
#define SYS_unlink     21
#define SYS_mmap       22
#define SYS_munmap     23
#define SYS_clone      24
#define SYS_exit_group 25
#define SYS_join       26
#define SYS_detach     27
#define SYS_kill       28
#define SYS_cv_open    29
#define SYS_cv_wait    30
#define SYS_cv_sig     31
#define SYS_cv_sigall  32
#define SYS_cv_close   33
#define SYS_pipe       34
#define SYS_mkfifo     35
#define SYS_link       36
#define SYS_symlink    37
#define SYS_spinlock_open    38
#define SYS_spinlock_acquire 39
#define SYS_spinlock_release 40
#define SYS_spinlock_close   41

#define NR_SYS         42

#endif
