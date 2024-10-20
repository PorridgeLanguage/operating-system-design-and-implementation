#include "ulib.h"

#define BUF_SIZE 5 // buffer size
#define PRODUCER_NUM 4
#define COMSUMER_NUM 2

int mutex, cv;

typedef struct monitor
{
    int buffer_size;
    int count; // the number of products in buffer
} monitor_t;

monitor_t *monitor_ptr;

int fork_s() {
  int pid = fork();
  assert(pid >= 0);
  return pid;
}

void produce_one() {
  sleep(rand() % 5 + 10);
}

void consume_one() {
  sleep(rand() % 10 + 5);
}

void producer(int id) {
  while (1) {
    produce_one();
    P(mutex); // producer enters monitor
    while(monitor_ptr->count == monitor_ptr->buffer_size){ // producer waiting condition
        cv_wait(cv, mutex);
        P(mutex);
    }
    // put to buffer
    printf("producer %d: produce\n", id);
    // modify condition
    monitor_ptr->count++;
    cv_sigall(cv);
    V(mutex); // leave monitor
  }
}

void consumer(int id) {
  while (1) {
    P(mutex); // consumer enters monitor
    while(monitor_ptr->count == 0){ // consumer waiting condition
        cv_wait(cv, mutex);
    }
    // put to buffer
    printf("consumer %d: consume\n", id);
    monitor_ptr->count--;
    cv_sigall(cv);
    V(mutex); // leave monitor
    consume_one();
  }
}

int main(){
    mutex = sem_open(1);
    cv = cv_open();

    monitor_ptr = mmap();
    monitor_ptr->buffer_size = BUF_SIZE;
    monitor_ptr->count = 0;
    
    printf("producer-consumer start\n");
  for (int i = 0; i < PRODUCER_NUM; ++i) {
    if (fork_s() == 0) {
      srand(i + 1);
      producer(i);
    }
  }
  for (int i = 0; i < COMSUMER_NUM; ++i) {
    if (fork_s() == 0) {
      srand(~i);
      consumer(i);
    }
  }
  while (1) ;
}