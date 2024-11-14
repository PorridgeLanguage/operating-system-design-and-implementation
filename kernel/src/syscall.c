#include "cte.h"
#include "file.h"
#include "klib.h"
#include "loader.h"
#include "proc.h"
#include "serial.h"
#include "sysnum.h"
#include "timer.h"
#include "vme.h"

typedef int (*syshandle_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

extern void* syscall_handle[NR_SYS];

void do_syscall(Context* ctx) {
  // TODO: WEEK2-interrupt call specific syscall handle and set ctx register
  int sysnum = ctx->eax;
  uint32_t arg1 = ctx->ebx;
  uint32_t arg2 = ctx->ecx;
  uint32_t arg3 = ctx->edx;
  uint32_t arg4 = ctx->esi;
  uint32_t arg5 = ctx->edi;
  int res;
  if (sysnum < 0 || sysnum >= NR_SYS) {
    res = -1;
  } else {
    res = ((syshandle_t)(syscall_handle[sysnum]))(arg1, arg2, arg3, arg4, arg5);
  }
  ctx->eax = res;
}

int sys_write(int fd, const void* buf, size_t count) {
  // TODO: rewrite me at Lab3-1
  // printf("WRITE\n");
  proc_t* cur_proc = proc_curr();
  if (cur_proc == NULL) {
    return -1;
  }
  cur_proc = cur_proc->group_leader;
  file_t* cur_file = proc_getfile(cur_proc, fd);
  // printf("FILE ADDRESS: %x\n", cur_file);
  if (cur_file == NULL) {
    return -1;
  }

  return fwrite(cur_file, buf, count);
  // return serial_write(buf, count);
}

int sys_read(int fd, void* buf, size_t count) {
  // TODO: rewrite me at Lab3-1
  proc_t* cur_proc = proc_curr();
  if (cur_proc == NULL) {
    return -1;
  }
  cur_proc = cur_proc->group_leader;
  file_t* cur_file = proc_getfile(cur_proc, fd);
  if (cur_file == NULL) {
    return -1;
  }
  return fread(cur_file, buf, count);
  // return serial_read(buf, count);
}

int sys_brk(void* addr) {
  // TODO: WEEK3-virtual-memory
  proc_t* proc = proc_curr();              // uncomment me in WEEK3-virtual-memory
  proc_t* leader = proc->group_leader;     // 获取主进程
  size_t brk = leader->brk;                // 使用主进程的 brk
  size_t new_brk = (size_t)PAGE_UP(addr);  // rewrite me
  if (brk == 0) {
    leader->brk = new_brk;  // 设置主线程的 brk
  } else if (new_brk > brk) {
    vm_map(leader->pgdir, brk, new_brk - brk, 7);  // 映射新的内存
    leader->brk = new_brk;                         // 更新主线程的 brk
  } else if (new_brk < brk) {
    // can just do nothing
    // recover memory, Lab 1 extend
  }
  return 0;
}

void sys_sleep(int ticks) {
  // TODO(); // WEEK2-interrupt
  uint32_t beg_tick = get_tick();
  while (get_tick() - beg_tick <= ticks) {
    // sti(); hlt(); cli(); // chage to me in WEEK2-interrupt
    proc_yield();  // change to me in WEEK4-process-api
                   // thread_yield();
  }
  return;
}

int sys_exec(const char* path, char* const argv[]) {
  // WEEK7: 线程相关
  PD* pg_new = vm_alloc();
  if (pg_new == NULL) {
    return -1;
  }
  proc_t* proc = proc_curr();
  proc_t* leader = proc->group_leader;
  if (load_user(pg_new, leader->ctx, path, argv) != 0) {
    vm_teardown(pg_new);
    return -1;
  }

  PD* pg_old = leader->pgdir;
  leader->pgdir = pg_new;  // 更新当前进程的页目录
  set_cr3(pg_new);         // 设置 CR3 寄存器为新页目录
  set_tss(KSEL(SEG_KDATA), (uint32_t)leader->kstack + PGSIZE);
  vm_teardown(pg_old);

  proc_t* thread = leader->thread_group;
  while (thread != NULL) {
    thread_free(thread);
    thread = thread->thread_group;
  }

  leader->thread_group = NULL;
  leader->thread_num = 1;
  proc_run(leader);

  // Default
  printf("sys_exec is not implemented yet.");
  while (1)
    ;
}

int sys_getpid() {
  // TODO(); // WEEK3-virtual-memory
  proc_t* current_proc = proc_curr();
  return current_proc->tgid;
}

int sys_gettid() {
  // TODO(); // Lab2-1
  proc_t* current_proc = proc_curr();
  return current_proc->pid;
}

void sys_yield() {
  proc_yield();
}

int sys_fork() {
  // TODO(); // WEEK4-process-api
  proc_t* child_pcb = proc_alloc();
  if (child_pcb == NULL) {
    return -1;
  }
  proc_copycurr(child_pcb);
  proc_addready(child_pcb);
  return child_pcb->pid;
}

void sys_exit(int status) {
  // TODO();
  proc_t* curr_thread = proc_curr();  // 获取当前线程

  if (curr_thread->pid == curr_thread->tgid) {
    // 当前线程是主线程
    while (curr_thread->thread_num > 1) {
      proc_yield();
    }
    assert(curr_thread->thread_num == 1);
    proc_t* thread;
    thread = curr_thread->thread_group;
    while (thread != NULL) {
      proc_t* next_thread = thread->thread_group;
      thread_free(thread);
      thread = next_thread;
    }

    proc_makezombie(curr_thread, status);
  } else {
    // 当前线程是普通线程
    if (curr_thread->detached == 1) {
      // 删除控制块
      proc_t* thread = curr_thread->group_leader;
      while (thread != NULL) {
        if (thread->thread_group == curr_thread) {
          thread->thread_group = curr_thread->thread_group;
          break;
        }
        thread = thread->thread_group;
      }
      proc_set_kernel_parent(curr_thread);
    }
    proc_t* leader = curr_thread->group_leader;  // 获取主线程
    leader->thread_num--;                        // 主线程的线程数量减1
    proc_makezombie(curr_thread, status);        // 将当前线程标记为 ZOMBIE
  }
  INT(0x81);
}

void sys_exit_group(int status) {
  // TODO();
  // WEEK4 process api
  proc_t* curr_proc = proc_curr();
  // WEEK6 thread_free
  proc_t* thread;
  thread = curr_proc->thread_group;
  while (thread != NULL) {
    proc_t* next_thread = thread->thread_group;
    thread_free(thread);
    thread = next_thread;
  }

  proc_makezombie(curr_proc, status);
  INT(0x81);
  assert(0);
}

int sys_wait(int* status) {
  // TODO(); // WEEK4 process api
  proc_t* curr_proc = proc_curr();
  proc_t* leader = curr_proc->group_leader;

  if (leader->child_num == 0) {
    return -1;
  }

  sem_p(&leader->zombie_sem);
  proc_t* zombie_child = proc_findzombie(leader);
  while (zombie_child == NULL) {
    proc_yield();
    zombie_child = proc_findzombie(leader);
  }
  if (status != NULL) {
    *status = zombie_child->exit_code;
  }
  int pid = zombie_child->pid;
  proc_free(zombie_child);

  leader->child_num--;
  return pid;
}

int sys_sem_open(int value) {
  // WEEK5-semaphore
  proc_t* curr_proc = proc_curr();
  proc_t* leader = curr_proc->group_leader;
  int idx = proc_allocusem(leader);
  if (idx == -1) {
    return -1;
  }
  usem_t* user_sem = usem_alloc(value);
  if (user_sem == NULL) {
    return -1;
  }
  leader->usems[idx] = user_sem;
  return idx;
}

int sys_sem_p(int sem_id) {
  // WEEK5-semaphore
  proc_t* curr_proc = proc_curr();
  proc_t* leader = curr_proc->group_leader;
  usem_t* user_sem = proc_getusem(leader, sem_id);
  if (user_sem == NULL) {
    return -1;
  }
  sem_p(&user_sem->sem);
  return 0;
}

int sys_sem_v(int sem_id) {
  // WEEK5-semaphore
  proc_t* curr_proc = proc_curr();
  proc_t* leader = curr_proc->group_leader;
  usem_t* user_sem = proc_getusem(leader, sem_id);
  if (user_sem == NULL) {
    return -1;
  }
  sem_v(&user_sem->sem);
  return 0;
}

int sys_sem_close(int sem_id) {
  // WEEK5-semaphore
  proc_t* curr_proc = proc_curr();
  proc_t* leader = curr_proc->group_leader;
  usem_t* user_sem = proc_getusem(leader, sem_id);
  if (user_sem == NULL) {
    return -1;
  }
  usem_close(user_sem);
  leader->usems[sem_id] = NULL;
  return 0;
}

int sys_open(const char* path, int mode) {
  proc_t* cur_proc = proc_curr();
  if (cur_proc == NULL) {
    return -1;
  }
  cur_proc = cur_proc->group_leader;
  int fd = proc_allocfile(cur_proc);
  if (fd != -1) {
    file_t* cur_file = fopen(path, mode);
    if (cur_file == NULL) {
      return -1;
    }
    cur_proc->files[fd] = cur_file;
    return fd;
  }
  return -1;
}

int sys_close(int fd) {
  proc_t* cur_proc = proc_curr();
  if (cur_proc == NULL) {
    return -1;
  }
  cur_proc = cur_proc->group_leader;
  file_t* cur_file = proc_getfile(cur_proc, fd);
  if (cur_file == NULL) {
    return -1;
  }
  fclose(cur_file);
  cur_proc->files[fd] = NULL;
  return 0;
}

int sys_dup(int fd) {
  proc_t* cur_proc = proc_curr()->group_leader;
  int new_fd = proc_allocfile(cur_proc);
  if (new_fd == -1) {
    return -1;
  }
  file_t* cur_file = proc_getfile(cur_proc, fd);
  if (cur_file == NULL) {
    return -1;
  }
  cur_proc->files[new_fd] = cur_file;
  fdup(cur_proc->files[new_fd]);
  return new_fd;
}

uint32_t sys_lseek(int fd, uint32_t off, int whence) {
  proc_t* cur_proc = proc_curr();
  if (cur_proc == NULL) {
    return -1;
  }
  cur_proc = cur_proc->group_leader;
  file_t* cur_file = proc_getfile(cur_proc, fd);
  if (cur_file == NULL) {
    return -1;
  }
  return fseek(cur_file, off, whence);
}

int sys_fstat(int fd, struct stat* st) {
  proc_t* cur_proc = proc_curr();
  if (cur_proc == NULL) {
    return -1;
  }
  cur_proc = cur_proc->group_leader;
  file_t* cur_file = proc_getfile(cur_proc, fd);
  if (cur_file == NULL) {
    return -1;
  }
  if (cur_file->type == TYPE_FILE) {
    st->type = itype(cur_file->inode);
    st->size = isize(cur_file->inode);
    st->node = ino(cur_file->inode);
  }
  if (cur_file->type == TYPE_DEV) {
    st->type = TYPE_DEV;
    st->size = 0;
    st->node = 0;
  }
  return 0;
}

int sys_chdir(const char* path) {
  // Lab3-2
  inode_t* cur_dir = iopen(path, TYPE_NONE);
  if (cur_dir == NULL) {
    return -1;
  }
  if (itype(cur_dir) != TYPE_DIR) {
    iclose(cur_dir);
    return -1;
  }
  proc_t* cur_proc = proc_curr();
  if (cur_proc->cwd != NULL) {
    iclose(cur_proc->cwd);
  }
  cur_proc->cwd = cur_dir;
  return 0;
}

int sys_unlink(const char* path) {
  return iremove(path);
}

// optional syscall

void* sys_mmap() {
  // TODO();
  // proc_t *proc = proc_curr(); // 获取当前进程
  for (size_t mmap_va = USR_MEM; mmap_va < VIR_MEM; mmap_va += PGSIZE) {
    // 检查该虚拟地址是否已经被占用
    if (vm_walkpte(vm_curr() /*proc->pgdir*/, mmap_va, 0) == NULL) {
      // 找到了空的虚拟页, 现在为该虚拟页分配物理内存
      void* new_page = kalloc();  // 分配一页物理内存
      if (new_page == NULL) {
        return NULL;  // 分配失败，返回 NULL
      }
      vm_map(vm_curr() /*proc->pgdir*/, mmap_va, PGSIZE, 7);
      return (void*)mmap_va;
    }
  }
  return NULL;
}

void sys_munmap(void* addr) {
  // TODO();
  vm_unmap(vm_curr(), (size_t)addr, PGSIZE);
}

int sys_clone(int (*entry)(void*), void* stack, void* arg, void (*ret_entry)(void)) {
  // 获取当前进程的组长进程
  proc_t* proc = proc_curr()->group_leader;

  // 分配新的进程
  proc_t* new_proc = proc_alloc();
  if (new_proc == NULL) {
    return -1;
  }

  // 设置tgid和group_leader
  new_proc->tgid = proc->tgid;
  new_proc->group_leader = proc;

  // 维护线程链表
  new_proc->thread_group = proc->thread_group;
  proc->thread_group = new_proc;

  // 普通线程的父进程设置为 NULL
  if (new_proc->pid != new_proc->tgid) {
    new_proc->parent = NULL;
  }

  // 增加线程数
  proc->thread_num++;

  // 使用主进程的页表
  new_proc->pgdir = proc->pgdir;

  // 设置用户栈
  uint32_t* stack_top = (uint32_t*)stack;
  stack_top--;
  *stack_top = (uint32_t)arg;
  stack_top--;
  *stack_top = (uint32_t)ret_entry;

  // 设置上下文
  new_proc->ctx->eip = (uint32_t)entry;
  new_proc->ctx->esp = (uint32_t)stack_top;
  new_proc->ctx->cs = USEL(SEG_UCODE);
  new_proc->ctx->ds = USEL(SEG_UDATA);
  new_proc->ctx->ss = USEL(SEG_UDATA);
  new_proc->ctx->eflags = 0x202;

  // 将新进程加入就绪队列
  proc_addready(new_proc);

  return new_proc->pid;
}

int sys_join(int tid, void** retval) {
  proc_t* cur_thread = proc_curr();
  proc_t* target_thread = pid2proc(tid);
  if (cur_thread->pid == target_thread->pid) {
    return 3;  // 自己join自己，返回ESRCH
  }

  if (target_thread->joinable == 0) {
    return 3;  // 目标线程不可以被join
  }
  target_thread->joinable = 0;  // 设置为不可再被join
  sem_p(&target_thread->join_sem);
  if (retval != NULL) {
    *retval = (void*)target_thread->exit_code;
  }
  return 0;
}

int sys_detach(int tid) {
  return thread_detach(tid);
}

int sys_kill(int pid, int signo) {
  proc_t* proc = pid2proc(pid);  // 得到指针控制块
  if (proc == NULL) {
    return 3;  // ESRCH: No such process
  }
  if (signo < 0 || signo >= SIGNAL_NUM) {
    return 22;  // EINVAL: Invalid signal
  }
  if (signo == SIGSTOP || signo == SIGCONT || signo == SIGKILL) {
    proc->sigaction[signo](signo, proc);
  } else {  // 处理可以缓一缓的信号
    // 检测sigpending_queue中是否已经有该信号量
    list_t* current = proc->sigpending_queue.next;
    while (current != &proc->sigpending_queue) {
      if ((int)(uintptr_t)current->ptr == signo) {
        return 0;  // 信号量已经挂起，直接返回
      }
      current = current->next;
    }
    list_enqueue(&proc->sigpending_queue, (void*)(uintptr_t)signo);
  }
  return 0;
}

int sys_cv_open() {
  return sys_sem_open(0);
}

int sys_cv_wait(int cv_id, int sem_id) {
  sys_sem_v(sem_id);
  sys_sem_p(cv_id);
  return 0;
}

int sys_cv_sig(int cv_id) {
  proc_t* cur_proc = proc_curr();
  usem_t* cur_usem = proc_getusem(cur_proc, cv_id);
  if (cur_usem == NULL) {
    return -1;
  }
  if (cur_usem->sem.value < 0) {
    sem_v(&cur_usem->sem);
  }
  return 0;
}

int sys_cv_sigall(int cv_id) {
  proc_t* cur_proc = proc_curr();
  usem_t* cur_usem = proc_getusem(cur_proc, cv_id);
  if (cur_usem == NULL) {
    return -1;
  }
  while (cur_usem->sem.value < 0) {
    sem_v(&cur_usem->sem);
  }
  return 0;
}

int sys_cv_close(int cv_id) {
  return sys_sem_close(cv_id);
}

int sys_pipe(int fd[2]) {
  TODO();
}

int sys_mkfifo(const char* path, int mode) {
  TODO();
}

int sys_link(const char* oldpath, const char* newpath) {
  TODO();
}

int sys_symlink(const char* oldpath, const char* newpath) {
  TODO();
}

int sys_sigaction(int signo, const void* act, void** oldact) {
  // WEEK8-signal: set new signal action handler
  if (signo < 0 || signo >= SIGNAL_NUM) {
    return 22;  // EINVAL: Invalid signal
  }
  proc_t* curr_proc = proc_curr();
  if (oldact != NULL) {
    *oldact = curr_proc->sigaction[signo];
  }
  // 更新信号量处理函数
  curr_proc->sigaction[signo] = (void (*)(int, proc_t*))act;
  return 0;
}

int sys_sigprocmask(int how, const int set, int* oldset) {
  // WEEK8-signal: set new signal action handler
  proc_t* curr_proc = proc_curr();

  if (oldset != NULL) {
    // 保存处理前的信号屏蔽字
    *oldset = curr_proc->sigblocked;
  }
  if (how == SIG_BLOCK) {
    curr_proc->sigblocked |= set;  // Block the signals in 'set'
  }
  if (how == SIG_UNBLOCK) {
    curr_proc->sigblocked &= ~set;  // Unblock the signals in 'set'
  }
  if (how == SIG_SETMASK) {
    curr_proc->sigblocked = set;  // Set the mask to 'set'
  }

  return 0;
}

void* syscall_handle[NR_SYS] = {
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
    [SYS_sigaction] = sys_sigaction,
    [SYS_sigprocmask] = sys_sigprocmask
    // [SYS_spinlock_open] = sys_spinlock_open,
    // [SYS_spinlock_acquire] = sys_spinlock_acquire,
    // [SYS_spinlock_release] = sys_spinlock_release,
    // [SYS_spinlock_close] = sys_spinlock_close,
};
