#include "ulib.h"

#define PHI_NUM 5

void init();
void philosopher(int id);

void eat(int id) {
  printf("philosopher %d start eating.\n", id);
  sleep(rand() % 10 + 5);
  printf("philosopher %d end eating.\n", id);
}

void think(int id) {
  //printf("philosopher %d start thinking.\n", id);
  sleep(rand() % 5 + 10);
  //printf("philosopher %d end thinking.\n", id);
}

int fork_s() {
  int pid = fork();
  assert(pid >= 0);
  return pid;
}

// The philosoper always take the fork with a large number

typedef struct monitor{
    size_t forks[PHI_NUM];
} monitor_t;

monitor_t *monitor_ptr;

uint16_t mutex, cv;

void init() {
  monitor_ptr = mmap();
  for(int i = 0; i < PHI_NUM; i++) monitor_ptr->forks[i] = 0;// In the beginning, the forks are not occupied.
  mutex = sem_open(1);
  cv = cv_open();
}

void philosopher(int id) {
  // implement philosopher, remember to call `eat` and `think`
  uint32_t lhf = id;
  uint32_t rhf = (id + 1) % 5;
  while (1) {
    think(id);

    P(mutex);
    while(monitor_ptr->forks[lhf] || monitor_ptr->forks[rhf]) {
        cv_wait(cv, mutex);
        P(mutex);
    }
    monitor_ptr->forks[lhf] = 1;
    monitor_ptr->forks[rhf] = 1;
    cv_sigall(cv);
    V(mutex);

    eat(id);

    P(mutex);
    monitor_ptr->forks[lhf] = 0;
    monitor_ptr->forks[rhf] = 0;
    cv_sigall(cv);
    V(mutex);
  }
}


int main() {
  init();
  printf("philosopher start.\n");
  for (int i = 0; i < PHI_NUM; ++i) {
    if (fork_s() == 0) {
      srand(i + 1);
      philosopher(i);
    }
  }
  while (1) ;
}
