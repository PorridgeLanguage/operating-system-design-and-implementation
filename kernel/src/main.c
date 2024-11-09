#include "cte.h"
#include "dev.h"
#include "fs.h"
#include "klib.h"
#include "loader.h"
#include "proc.h"
#include "serial.h"
#include "timer.h"
#include "vme.h"

void init_user_and_go();

int main() {
  init_gdt();
  init_serial();
  init_fs();
  init_page();   // uncomment me at WEEK3-virtual-memory
  init_cte();    // uncomment me at WEEK2-interrupt
  init_timer();  // uncomment me at WEEK2-interrupt
  init_proc();   // uncomment me at WEEK1-os-start
  init_dev();    // uncomment me at Lab3-1
  printf("Hello from OS!\n");
  init_user_and_go();
  panic("should never come back");
}

void init_user_and_go() {
  proc_t* proc = proc_alloc();
  assert(proc);
  char* argv[] = {"sh", NULL};
  assert(load_user(proc->pgdir, proc->ctx, "sh", argv) == 0);
  proc_addready(proc);

  sti();
  // WEEK7-thread: kernel free all isolated processes or detached threads
  proc_t* kernel = proc_curr();
  // infinit loop
  while (1) {
    proc_t* proc_child;
    // Don't use zombie_sem cause there should always be one process being runnable.
    cli();  // close interrupt first

    while (!(proc_child = proc_findzombie(kernel))) {
      sti();
      proc_yield();
    }
    if (proc_child->pid == proc_child->tgid) {
      proc_free(proc_child);  // 是主进程，释放进程
    } else {
      thread_free(proc_child);  // 释放当前进程
    }
    kernel->child_num--;
  }
}
