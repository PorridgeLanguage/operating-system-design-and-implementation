#ifndef __PROC_H__
#define __PROC_H__

#include "klib.h"
#include "vme.h"
#include "cte.h"
#include "sem.h"
#include "file.h"

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
  size_t entry; // the address of the process entry, this can be removed after WEEK2-interrupt
  size_t pid;
  enum {UNUSED, UNINIT, RUNNING, READY, ZOMBIE, BLOCKED} status;
  // WEEK2-interrupt
  //kstack_t *kstack;
  //Context *ctx; // points to restore context for READY proc
  // WEEK3-virtual-memory
  //PD *pgdir; 
  //size_t brk;
  // WEEK4-process-api
  //struct proc *parent; 
  //int child_num; 
  //int exit_code;
  // WEEK5-semaphore
  //sem_t zombie_sem; 
  //usem_t *usems[MAX_USEM];


  //file_t *files[MAX_UFILE]; // Lab3-1
  //inode_t *cwd; // Lab3-2
} proc_t;

void init_proc();
proc_t *proc_alloc();
void proc_free(proc_t *proc);
proc_t *proc_curr();
void proc_run(proc_t *proc); // __attribute__((noreturn));
void proc_addready(proc_t *proc);
void proc_yield();
void proc_copycurr(proc_t *proc);
void proc_makezombie(proc_t *proc, int exitcode);
proc_t *proc_findzombie(proc_t *proc);
void proc_block();
int proc_allocusem(proc_t *proc);
usem_t *proc_getusem(proc_t *proc, int sem_id);
int proc_allocfile(proc_t *proc);
file_t *proc_getfile(proc_t *proc, int fd);

void schedule(Context *ctx);

#endif
