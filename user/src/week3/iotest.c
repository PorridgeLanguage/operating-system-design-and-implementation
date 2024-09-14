#include "ulib.h"

void test() {
  printf("iotest start.\n");
  printf("input two numbers: $ ");
  int a, b;
  scanf("%d%d", &a, &b);
  printf("%d + %d = %d\n", a, b, a + b);
  printf("iotest passed!\n");
}

int main() {
  test();
  
  //while(1); 
  
  // If you want to use sh1, uncomment me.
  char *sh1_argv[] = {"sh1", NULL};
  exec("sh1", sh1_argv);
  assert(0);
  
  return 0;
}
