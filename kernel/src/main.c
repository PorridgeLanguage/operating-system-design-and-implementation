#include "klib.h"
#include "serial.h"
#include "vme.h"
#include "cte.h"
#include "loader.h"
#include "fs.h"
#include "proc.h"
#include "timer.h"
#include "dev.h"

void init_user_and_go();

int main() {
  init_gdt();
  init_serial();
  init_fs();
  init_page(); // uncomment me at WEEK3-virtual-memory
  init_cte(); // uncomment me at WEEK2-interrupt
  init_timer(); // uncomment me at WEEK2-interrupt
  init_proc(); // uncomment me at WEEK1-os-start
  //init_dev(); // uncomment me at Lab3-1
  printf("Hello from OS!\n");
  init_user_and_go();
  panic("should never come back");
}

void init_user_and_go() {

  proc_t *proc = proc_alloc();
  assert(proc);
  // char *argv[] = {"ping3", "114514", "1919810", NULL};
  // assert(load_user(proc->pgdir, proc->ctx, "ping3", argv) == 0);
  char *argv[] = {"sh", NULL};
  assert(load_user(proc->pgdir, proc->ctx, "sh", argv) == 0);
  proc_addready(proc);

  sti();
  while(1);
}
