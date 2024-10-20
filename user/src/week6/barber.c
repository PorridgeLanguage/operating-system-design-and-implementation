#include "barber.h"

// WEEK6-synchronization: define the shared-memory variables and semaphores you need.

int *waiting_ptr;
int customers, barbers, mutex;

void init() {
  // init some sem if you need
  // TODO();

  /* init shared memory */
  waiting_ptr = mmap();
  
  /* init semaphores */
  customers = sem_open(0);
  barbers = sem_open(0);
  mutex = sem_open(1);
}

void barber() {
  // implement barber, remember to call `cuthair`
  while (1) {
    // TODO();
    P(customers);
    P(mutex);
    (*waiting_ptr)--;
    V(barbers);
    V(mutex);
    cuthair();
  }
}

void customer(int id) {
  // implement customer, remember to call `get_haircut` and `leave`
  // The customer will not come back again, so there is no loop.
  // TODO();
  P(mutex);
  if(*waiting_ptr < CHAIRS_NUM){
    (*waiting_ptr)++;
    find_a_chair(id);
    V(customers);
    V(mutex);
    P(barbers);
    get_haircut(id);
  }
  else{
    V(mutex);
    leave(id);
  }


  sem_close(mutex);
  sem_close(customers);
  sem_close(barbers);
}