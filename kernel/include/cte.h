#ifndef __CTE_H__
#define __CTE_H__

#include <stdint.h>

// TODO: WEEK2: adjust the struct to the correct order
// TODO: WEEK2: add esp and ss
typedef struct Context {
  uint32_t ds;
  uint32_t ebp;
  uint32_t edi;
  uint32_t esi;
  uint32_t edx;
  uint32_t ecx;
  uint32_t ebx;
  uint32_t eax;
  uint32_t irq;
  uint32_t errcode;
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
  uint32_t esp, ss;
} Context;

void init_cte();
void irq_iret(Context *ctx) __attribute__((noreturn));

void do_syscall(Context *ctx);
void exception_debug_handler(Context *ctx);

#endif
