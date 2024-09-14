#ifndef __CTE_H__
#define __CTE_H__

#include <stdint.h>

// TODO: WEEK2: adjust the struct to the correct order
// TODO: WEEK2: add esp and ss
typedef struct Context {
  uint32_t eax, ebx, ecx, edx, esi, 
           edi, ebp, eip, cs, ds, 
           eflags, irq, errcode;
} Context;

void init_cte();
void irq_iret(Context *ctx) __attribute__((noreturn));

void do_syscall(Context *ctx);
void exception_debug_handler(Context *ctx);

#endif
