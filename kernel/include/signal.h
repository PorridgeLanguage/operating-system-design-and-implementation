#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include "klib.h"

// WEEK8-signal
#define SIGNAL_NUM 6

#define SIGSTOP 0  // unblockable
#define SIGCONT 1  // unblockable
#define SIGKILL 2  // unblockable
#define SIGUSR1 3  // blockable, user-defined
#define SIGUSR2 4  // blockable, user-defined
#define SIGUSR3 5  // blockable, user-defined

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#endif
