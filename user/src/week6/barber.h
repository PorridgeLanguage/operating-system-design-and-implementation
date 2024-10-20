#include "ulib.h"

// There are 20 customers and 1 barber whose barber shop has 5 chairs.

#define CUSTOMER_NUM 20 

#define CHAIRS_NUM 5

void init();
void barber();
void customer(int id);

void cuthair() {
  printf("The barber starts cutting hair.\n");
  sleep(rand() % 10 + 5);
  printf("The barber ends cutting hair.\n");
}

void get_haircut(int id) {
  printf("The customer %d gets hair cut.\n", id);
}

void leave(int id){
  printf("The customer %d just leaves for no empty chair.\n", id);
}

void find_a_chair(int id){
  printf("The customer %d just finds a chair to sit.\n", id);
}


int fork_s() {
  int pid = fork();
  assert(pid >= 0);
  return pid;
}

int main() {
  init();
  printf("sleeping-barber problem starts.\n");

  if (fork_s() == 0) {
    srand(2024);
    barber();
  }

  for (int i = 0; i < CUSTOMER_NUM; ++i) {
    if (fork_s() == 0) {
      srand(i + 1);
      customer(i);
      exit(0);
    }
  }

  while (1) ;
}