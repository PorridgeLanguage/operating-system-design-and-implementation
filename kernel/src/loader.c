#include "klib.h"
#include "vme.h"
#include "cte.h"
#include "loader.h"
#include "disk.h"
#include "fs.h"
#include <elf.h>

uint32_t load_elf(PD *pgdir, const char *name) {
  Elf32_Ehdr elf;
  Elf32_Phdr ph;
  inode_t *inode = iopen(name, TYPE_NONE);
  if (!inode) return -1;
  iread(inode, 0, &elf, sizeof(elf));
  if (*(uint32_t*)(&elf) != 0x464c457f) { // check ELF magic number
    iclose(inode);
    return -1;
  }

  // WEEK3-virtual-memory: Restore cr3 for convenient loading

  for (int i = 0; i < elf.e_phnum; ++i) {
    iread(inode, elf.e_phoff + i * sizeof(ph), &ph, sizeof(ph));
    if (ph.p_type == PT_LOAD) {
      // WEEK1: Load segment to physical memory
      // TODO();

      // WEEK3-virtual-memory: Load segment to virtual memory
      // TODO();
    }
  }
  
  // TODO: WEEK3-virtual-memory alloc stack memory in pgdir
  // WEEK3-virtual-memory: reset cr3

  iclose(inode);
  return elf.e_entry;
}

#define MAX_ARGS_NUM 31

uint32_t load_arg(PD *pgdir, char *const argv[]) {
  // WEEK2: Load argv to user stack directly in physical address
  char *stack_top = (char *)(KER_MEM - 2 * PGSIZE); // (char*)vm_walk(pgdir, USR_MEM - PGSIZE, 7) + PGSIZE;
  // char *stack_top = (char*)vm_walk(pgdir, USR_MEM - PGSIZE, 7) + PGSIZE; // change to me in WEEK3-virtual-memory

  size_t argv_va[MAX_ARGS_NUM + 1];
  int argc;
  for (argc = 0; argv && argv[argc]; ++argc) {
    assert(argc < MAX_ARGS_NUM);
    // WEEK2-interrupt: push the string of argv[argc] to stack, record its va to argv_va[argc]
    // WEEK3-virtual-memory: start virtual memory mechanism
    TODO();
  }
  argv_va[argc] = 0; // set last argv NULL
  stack_top -= ADDR2OFF(stack_top) % 4; // align to 4 bytes
  for (int i = argc; i >= 0; --i) {
    // push the address of argv_va[argc] to stack to make argv array
    stack_top -= sizeof(size_t);
    *(size_t*)stack_top = argv_va[i];
  }

  // WEEK2-interrupt: push the address of the argv array as argument for _start
  TODO();
   
  // WEEK3-virtual-memory: start virtual memory mechanism
  
  // push argc as argument for _start
  stack_top -= sizeof(size_t);
  *(size_t*)stack_top = argc;
  stack_top -= sizeof(size_t); // a hole for return value (useless but necessary)

  return (uint32_t)stack_top; 
  // return USR_MEM - PGSIZE + ADDR2OFF(stack_top); // change to me in WEEK3-virtual-memory
}

int load_user(PD *pgdir, Context *ctx, const char *name, char *const argv[]) {
  size_t eip = load_elf(pgdir, name);
  if (eip == -1) return -1;
  ctx->cs = USEL(SEG_UCODE);
  ctx->ds = USEL(SEG_UDATA);
  ctx->eip = eip;
  // TODO: WEEK2 init ctx->ss and esp
  // TODO: WEEK2 load arguments
  ctx->eflags = 0x002; // TODO: WEEK2-interrupt change me to 0x202
  return 0;
}
