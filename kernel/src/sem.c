#include "klib.h"
#include "sem.h"
#include "proc.h"

void sem_init(sem_t *sem, int value) {
  sem->value = value;
  list_init(&sem->wait_list);
}

void sem_p(sem_t *sem) {
  // WEEK5-semaphore: dec sem's value, if value<0, add curr proc to waitlist and block it
  // TODO();
  sem->value--;
  if (sem->value < 0) {
    list_enqueue(&(sem->wait_list), proc_curr());
    proc_block();
  }
}

void sem_v(sem_t *sem) {
  // WEEK5-semaphore: inc sem's value, if value<=0, dequeue a proc from waitlist and ready it
  // TODO();
  sem->value++;
  if (sem->value <= 0) {
    proc_t *proc_todo = list_dequeue(&(sem->wait_list));
    proc_addready(proc_todo);
  }
}

#define USER_SEM_NUM 128
static usem_t user_sem[USER_SEM_NUM] __attribute__((used));

usem_t *usem_alloc(int value) {
  // WEEK5-semaphore: find a usem whose ref==0, init it, inc ref and return it, return NULL if none
  for (int i = 0; i < USER_SEM_NUM; i++) {
    if (user_sem[i].ref == 0) {
      sem_init(&user_sem[i].sem, value);
      user_sem[i].ref++;
      return &user_sem[i];
    }
  }
  return NULL;
}

usem_t *usem_dup(usem_t *usem) {
  // WEEK5-semaphore: inc usem's ref
  usem->ref++;
  return usem;
}

void usem_close(usem_t *usem) {
  // WEEK5-semaphore: dec usem's ref
  usem->ref--;
}


