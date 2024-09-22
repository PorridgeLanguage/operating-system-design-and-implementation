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

  // WEEK3: virtual memory
  proc_t *proc = proc_alloc();
  assert(proc);
  char *argv[] = {"sh1", NULL}; 
  assert(load_user(proc->pgdir, proc->ctx, "sh1", argv) == 0);
  
  proc_run(proc);

}
