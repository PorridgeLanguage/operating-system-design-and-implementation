#include "ulib.h"

void test() {
  printf("sleeptest start.\n");
  sleep(100);
  printf("sleeptest passed!\n");
}

int main() {
  test();
  char *sh1_argv[] = {"sh1", NULL};
  exec("sh1", sh1_argv);
  assert(0);
  return 0;
}
