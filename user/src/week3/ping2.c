#include "ulib.h"

int main(int argc, char *argv[]) {
  int pid = getpid();
  int x = argc > 1 ? atoi(argv[1]) : 0;
  for (int i = 0; i < 8; ++i) {
    printf("ping: pid=%d, i=%d, x=%d\n", pid, i, x);
    sleep(25);
  }

  // while (1) ;

  // If you want to use sh1, uncomment me.
  char *sh1_argv[] = {"sh1", NULL};
  exec("sh1", sh1_argv);
  assert(0);

  return 0;
}
