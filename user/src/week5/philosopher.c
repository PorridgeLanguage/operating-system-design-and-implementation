#include "philosopher.h"

// TODO: define some sem if you need
int chopsticks[PHI_NUM];

void init() {
  // init some sem if you need
  for (int i = 0; i < PHI_NUM; i++) {
    chopsticks[i] = sem_open(1);
  }
}

void philosopher(int id) {
  // implement philosopher, remember to call `eat` and `think`
  while (1) {
    int left_chopstick = id;
    int right_chopstick = (id + 1) % PHI_NUM;

    if (left_chopstick < right_chopstick) {
      P(left_chopstick);
      P(right_chopstick);
    } else {
      P(right_chopstick);
      P(left_chopstick);
    }
    eat(id);
    V(left_chopstick);
    V(right_chopstick);
    think(id);
  }
}
