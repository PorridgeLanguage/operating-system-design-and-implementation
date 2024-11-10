#ifndef __PROC_H__
#define __PROC_H__

#include "cte.h"
#include "file.h"
#include "klib.h"
#include "sem.h"
#include "signal.h"
#include "vme.h"

#define KSTACK_SIZE 4096

typedef union {
  uint8_t stack[KSTACK_SIZE];
  struct {
    uint8_t pad[KSTACK_SIZE - sizeof(Context)];
    Context ctx;
  };
} kstack_t;

#define STACK_TOP(kstack) (&((kstack)->stack[KSTACK_SIZE]))
#define MAX_USEM 32
#define MAX_UFILE 32

// Forward declaration of proc_t to resolve cross-reference
typedef struct proc proc_t;

typedef struct proc {
  size_t entry;  // the address of the process entry, this can be removed after WEEK2-interrupt
  size_t pid;
  enum { UNUSED,
         UNINIT,
         RUNNING,
         READY,
         ZOMBIE,
         BLOCKED } status;
  // WEEK2-interrupt
  kstack_t* kstack;
  Context* ctx;  // points to restore context for READY proc
  // WEEK3-virtual-memory
  PD* pgdir;
  size_t brk;
  // WEEK4-process-api
  struct proc* parent;
  int child_num;
  int exit_code;
  // WEEK5-semaphore
  sem_t zombie_sem;
  usem_t* usems[MAX_USEM];
  // WEEK7-thread
  size_t tgid;                // 进程组ID
  int thread_num;             // 进程所有的线程数量
  struct proc* group_leader;  // 进程控制块指针，指向进程对应的主线程
  struct proc* thread_group;  // 链表指针，指向线程链表的下一个成员。

  // WEEK7-thread: join & detach
  int joinable;    // 指示该线程能否被join，初始化为1，代表默认是能够被join的。
  int detached;    // 指示该线程是否被detach了，初始化为0，代表默认是没有被detach的。
  sem_t join_sem;  // 维护join信息的信号量，初始化为value=0

  // WEEK8-signal
  int sigblocked;                                    // 整型数，以位图的形式指示信号是否被阻塞。
  void (*sigaction[SIGNAL_NUM])(int, struct proc*);  // 函数指针数组，指示不同信号对应的处理函数。
  list_t sigpending_queue;                           // 等待队列，维护收到但还没有来得及处理的信号。

  file_t* files[MAX_UFILE];  // WEEK9-files
  inode_t* cwd;              // Lab3-2
} proc_t;

void init_proc();
proc_t* proc_alloc();
void proc_free(proc_t* proc);
proc_t* proc_curr();
void proc_run(proc_t* proc);  // __attribute__((noreturn));
void proc_addready(proc_t* proc);
void proc_yield();
void proc_copycurr(proc_t* proc);
void proc_makezombie(proc_t* proc, int exitcode);
proc_t* proc_findzombie(proc_t* proc);
void proc_block();
int proc_allocusem(proc_t* proc);
usem_t* proc_getusem(proc_t* proc, int sem_id);
int proc_allocfile(proc_t* proc);
file_t* proc_getfile(proc_t* proc, int fd);

void schedule(Context* ctx);
void thread_free(proc_t* thread);
int thread_detach(int tid);
void proc_set_kernel_parent(proc_t* proc);
proc_t* pid2proc(int pid);

// WEEK8-signal
void do_signal(proc_t* proc);
void handle_signal(int signo, proc_t* proc);

#endif
