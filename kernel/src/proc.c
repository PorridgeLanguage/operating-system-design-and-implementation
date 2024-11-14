#include "proc.h"
#include "cte.h"
#include "klib.h"

#define PROC_NUM 64

static __attribute__((used)) int next_pid = 1;

proc_t pcb[PROC_NUM];
static proc_t* curr = &pcb[0];

void init_proc() {
  // WEEK1: init proc status
  pcb[0].status = RUNNING;
  // WEEK2: add ctx and kstack for interruption
  pcb[0].kstack = (void*)(KER_MEM - PGSIZE);
  pcb[0].ctx = &pcb[0].kstack->ctx;
  // WEEK3: add pgdir
  pcb[0].pgdir = vm_curr();
  // WEEK4: init parent and child_num
  pcb[0].parent = NULL;
  pcb[0].child_num = 0;
  // WEEK5: semaphore
  sem_init(&pcb[0].zombie_sem, 0);
  for (int i = 0; i < MAX_USEM; i++) {
    pcb[0].usems[i] = NULL;
  }
  // Lab2-1, set status and pgdir
  // Lab2-4, init zombie_sem
  // Lab3-2, set cwd
  // WEEK7: initialize thread-related fields
  pcb[0].tgid = pcb[0].pid;       // tgid == pid 默认为是主线程
  pcb[0].thread_num = 1;          // 线程数量为1
  pcb[0].group_leader = &pcb[0];  // group_leader指向自己
  pcb[0].thread_group = NULL;     // 后一个线程为NULL

  pcb[0].joinable = 1;
  pcb[0].detached = 0;
  sem_init(&(pcb[0].join_sem), 0);

  // 初始化信号相关变量
  pcb[0].sigblocked = 0;
  for (int i = 0; i < SIGNAL_NUM; i++) {
    pcb[0].sigaction[i] = handle_signal;
  }
  list_init(&pcb[0].sigpending_queue);

  // 初始化用户打开文件表
  for (int i = 0; i < MAX_UFILE; i++) {
    pcb[0].files[i] = NULL;
  }
  // 要设置内核进程pcb[0]的cwd为根目录
  pcb[0].cwd = iopen("/", TYPE_NONE);
}

proc_t* proc_alloc() {
  // WEEK1: alloc a new proc, find a unused pcb from pcb[1..PROC_NUM-1], return NULL if no such one
  for (int i = 1; i < PROC_NUM; i++) {
    if (pcb[i].status == UNUSED) {
      pcb[i].pid = next_pid++;
      pcb[i].status = UNINIT;

      // pcb[i].kstack = (kstack_t *)(KER_MEM - 2 * PGSIZE);
      pcb[i].pgdir = vm_alloc();
      pcb[i].kstack = kalloc();
      pcb[i].ctx = &pcb[i].kstack->ctx;

      // WEEK4: init parent and child_num
      pcb[i].parent = NULL;
      pcb[i].child_num = 0;

      // WEEK5: init zombie_sem
      sem_init(&pcb[i].zombie_sem, 0);
      for (int j = 0; j < MAX_USEM; j++) {
        pcb[i].usems[j] = NULL;
      }
      // WEEK7: initialize thread-related fields
      pcb[i].tgid = pcb[i].pid;       // tgid == pid 默认为是主线程
      pcb[i].thread_num = 1;          // 线程数量为1
      pcb[i].group_leader = &pcb[i];  // group_leader指向自己
      pcb[i].thread_group = NULL;     // 后一个线程为NULL

      pcb[i].joinable = 1;
      pcb[i].detached = 0;
      sem_init(&(pcb[i].join_sem), 0);

      // 初始化信号相关变量
      pcb[i].sigblocked = 0;
      for (int j = 0; j < SIGNAL_NUM; j++) {
        pcb[i].sigaction[j] = handle_signal;
      }
      list_init(&pcb[i].sigpending_queue);

      // 初始化用户打开文件表
      for (int j = 0; j < MAX_UFILE; j++) {
        pcb[i].files[j] = NULL;
      }
      pcb[i].cwd = NULL;
      // 返回新分配的进程控制块
      return &pcb[i];
    }
  }
  return NULL;
}

void proc_free(proc_t* proc) {
  // WEEK3-virtual-memory: free proc's pgdir and kstack and mark it UNUSED
  // TODO();
  proc->status = UNUSED;
  // vm_teardown(proc->pgdir);
  proc->pgdir = NULL;
  proc->kstack = NULL;
  proc->ctx = NULL;
  proc->pid = -1;
  proc->brk = 0;

  // 回收信号相关变量
  list_t* current = proc->sigpending_queue.next;
  while (current != &proc->sigpending_queue) {
    list_t* next = current->next;
    list_remove(&proc->sigpending_queue, current);
    current = next;
  }
}

proc_t* proc_curr() {
  return curr;
}

void proc_run(proc_t* proc) {
  // WEEK3: virtual memory
  proc->status = RUNNING;
  curr = proc;
  set_cr3(proc->pgdir);
  set_tss(KSEL(SEG_KDATA), (uint32_t)STACK_TOP(proc->kstack));
  do_signal(proc);  // 信号处理
  irq_iret(proc->ctx);
}

void proc_addready(proc_t* proc) {
  // WEEK4-process-api: mark proc READY
  // TODO();
  proc->status = READY;
}

void proc_yield() {
  // WEEK4-process-api: mark curr proc READY, then int $0x81
  curr->status = READY;
  INT(0x81);
}

void proc_copycurr(proc_t* proc) {
  // WEEK4-process-api: copy curr proc
  proc_t* leader = curr->group_leader;

  vm_copycurr(proc->pgdir);
  proc->brk = leader->brk;
  proc->kstack->ctx = curr->kstack->ctx;

  proc->ctx->eax = 0;

  proc->parent = leader;
  leader->child_num++;

  // WEEK5-semaphore: dup opened usems
  for (int i = 0; i < MAX_USEM; i++) {
    proc->usems[i] = leader->usems[i];
    if (leader->usems[i]) {
      proc->usems[i] = usem_dup(leader->usems[i]);
    }
  }

  // WEEK9: dup opened files
  for (int i = 0; i < MAX_UFILE; i++) {
    if (leader->files[i] != NULL) {
      proc->group_leader->files[i] = fdup(leader->files[i]);
    } else {
      proc->group_leader->files[i] = NULL;
    }
  }
  // Lab3-2: dup cwd
  if (leader->cwd != NULL) {
    proc->cwd = idup(leader->cwd);
  } else {
    proc->cwd = NULL;
  }
}

void proc_makezombie(proc_t* proc, int exitcode) {
  // WEEK4-process-api: mark proc ZOMBIE and record exitcode, set children's parent to NULL
  proc->status = ZOMBIE;
  proc->exit_code = exitcode;
  for (int i = 0; i < PROC_NUM; i++) {
    if (pcb[i].parent == proc) {
      pcb[i].parent = NULL;
      proc_set_kernel_parent(&pcb[i]);
    }
  }
  // WEEK5-semaphore: release parent's semaphore
  if (proc->parent) {
    sem_v(&proc->parent->zombie_sem);
  }
  // WEEK5-close proc's usem
  for (int i = 0; i < MAX_USEM; i++) {
    if (proc->usems[i]) {
      usem_close(proc->usems[i]);
    }
  }
  // WEEK7: 释放join_sem
  sem_v(&proc->join_sem);

  // WEEK9: close opened files
  if (proc->pid == proc->tgid) {
    // 主进程关闭，Maybe wrong
    for (int i = 0; i < MAX_UFILE; i++) {
      if (proc->files[i] != NULL) {
        fclose(proc->files[i]);
      }
    }
  }
  // WEEK10: close cwd
  if (proc->pid == proc->tgid && proc->cwd != NULL) {
    iclose(proc->cwd);
  } else {
    proc->cwd = NULL;
  }
}

proc_t* proc_findzombie(proc_t* proc) {
  // WEEK4-process-api: find a ZOMBIE whose parent is proc, return NULL if none
  // TODO();
  for (int i = 0; i < PROC_NUM; i++) {
    if (pcb[i].status == ZOMBIE && pcb[i].parent == proc) {
      return &pcb[i];
    }
  }
  return NULL;
}

void proc_block() {
  // WEEK4-process-api: mark curr proc BLOCKED, then int $0x81
  curr->status = BLOCKED;
  INT(0x81);
}

int proc_allocusem(proc_t* proc) {
  // WEEK7: 使用主进程改写函数
  proc_t* leader = proc->group_leader;  // 获取主进程
  for (int i = 0; i < MAX_USEM; i++) {
    if (leader->usems[i] == NULL) {
      return i;
    }
  }
  return -1;
}

usem_t* proc_getusem(proc_t* proc, int sem_id) {
  // WEEK7: 使用主进程改写函数
  if (sem_id < 0 || sem_id >= MAX_USEM) {
    return NULL;
  }
  return proc->group_leader->usems[sem_id];
}

int proc_allocfile(proc_t* proc) {
  // Lab3-1: find a free slot in proc->files, return its index, or -1 if none
  for (int i = 0; i < MAX_UFILE; i++) {
    if (proc->group_leader->files[i] == NULL) {
      return i;
    }
  }
  return -1;
}

file_t* proc_getfile(proc_t* proc, int fd) {
  // Lab3-1: return proc->files[fd], or NULL if fd out of bound
  if (fd < 0 || fd >= MAX_UFILE) {
    return NULL;
  }
  return proc->group_leader->files[fd];
}

void schedule(Context* ctx) {
  // WEEK4-process-api: save ctx to curr->ctx, then find a READY proc and run it
  // TODO();
  curr->ctx = ctx;
  int next_proc = (curr - pcb + 1) % PROC_NUM;
  for (int i = 0; i < PROC_NUM; i++) {
    if (pcb[next_proc].status == READY) {
      proc_run(&pcb[next_proc]);
      return;
    }
    next_proc = (next_proc + 1) % PROC_NUM;
  }
  proc_run(curr);
}

void thread_free(proc_t* thread) {
  if (thread == NULL) {
    return;
  }
  // 回收信号相关变量
  list_t* current = thread->sigpending_queue.next;
  while (current != &thread->sigpending_queue) {
    list_t* next = current->next;
    list_remove(&thread->sigpending_queue, current);
    current = next;
  }
  memset(thread, 0, sizeof(proc_t));
}

int thread_detach(int tid) {
  for (int i = 0; i < PROC_NUM; i++) {
    if (pcb[i].pid == tid) {
      pcb[i].detached = 1;
      pcb[i].joinable = 0;
      return 0;
    }
  }
  return -1;
}

void proc_set_kernel_parent(proc_t* proc) {
  proc->parent = pcb;
  pcb->child_num += 1;
}

proc_t* pid2proc(int pid) {
  for (int i = 0; i < PROC_NUM; i++) {
    if (pcb[i].pid == pid) {
      return &pcb[i];
    }
  }
  return NULL;
}

// WEEK8-signal
void do_signal(proc_t* proc) {
  list_t* current = proc->sigpending_queue.next;

  // 遍历sigpending_queue
  while (current != &proc->sigpending_queue) {
    int signo = (int)(intptr_t)current->ptr;  // 从链表节点中获取信号编号

    // 检查信号是否被阻塞
    if (!(proc->sigblocked & (1 << signo))) {  // 如果信号没有被阻塞
      // 调用对应的信号处理函数
      if (proc->sigaction[signo] != NULL) {
        proc->sigaction[signo](signo, proc);
      }

      // 从sigpending_queue中移除该信号
      list_remove(&proc->sigpending_queue, current);
      break;  // 每次只处理一个信号
    }

    current = current->next;  // 继续下一个信号
  }
}

void handle_signal(int signo, proc_t* proc) {
  // WEEK8-signal
  assert(signo >= 0 && signo < SIGNAL_NUM);
  switch (signo) {
    case SIGSTOP:
      // Handle SIGHUP logic
      proc->status = BLOCKED;
      if (proc == curr) {
        INT(0x81);  // 被停止的进程恰好现在正在运行，让出CPU
      }
      break;

    case SIGCONT:
      // TODO: Implement SIGCONT logic here
      proc_addready(proc);
      break;

    case SIGKILL:
      // Handle SIGKILL signal
      if (proc == NULL || proc->pid != proc->tgid) {
        return;
      }
      proc_t* thread = proc->thread_group;
      while (thread != NULL) {
        thread_free(thread);
        thread = thread->thread_group;
      }
      proc_makezombie(proc, 9);
      if (proc == proc_curr()) {
        INT(0x81);
      }
      break;

    case SIGUSR1:
      printf("Signal SIGUSR1 in proc %d is not defined.\n", proc_curr()->tgid);
      break;

    case SIGUSR2:
      printf("Signal SIGUSR2 in proc %d is not defined.\n", proc_curr()->tgid);
      break;

    default:
      printf("Received an invalid signal number: %d\n", signo);
      panic("Signal error");
      break;
  }
}
