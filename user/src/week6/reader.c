#include "ulib.h"

#define BUF_SIZE 32
#define READER_NUM 3
#define WRITER_NUM 2

int writeblock, mutex;

int *readcount_ptr;
void *share_memory;

int fork_s() {
  int pid = fork();
  assert(pid >= 0);
  return pid;
}

void read_file() {
  sleep(rand() % 5 + 10);
}

void write_file() {
  sleep(rand() % 10 + 5);
}

void rest(){
  sleep(rand() % 10 + 5);
}

void reader(int id) {
  while (1) {
    P(mutex);
    (*readcount_ptr)++;
    if(*readcount_ptr == 1){
        P(writeblock);
    }
    V(mutex);
    /* read file */
    read_file();
    printf("Reader %d is reading file.\n", id);
    (*readcount_ptr)--;
    if(*readcount_ptr == 0){
        V(writeblock);
    }
    V(mutex);
    rest();
  }
}

void writer(int id) {
  while (1) {
    P(writeblock);
    /* write file */
    write_file();
    printf("Writer %d is writing file.\n", id);
    V(writeblock);
  }
}

int main() {
  /* allocate memory space for readcount */
  share_memory = mmap();
  readcount_ptr = (int *)share_memory;

  /* init semaphores */
  writeblock = sem_open(1);
  mutex = sem_open(1);

  printf("reader-writer start\n");
  for (int i = 0; i < WRITER_NUM; ++i) {
    if (fork_s() == 0) {
      srand(~i);
      writer(i);
    }
  }
  for (int i = 0; i < READER_NUM; ++i) {
    if (fork_s() == 0) {
      srand(i + 1);
      reader(i);
    }
  }
  while (1) ;
}
