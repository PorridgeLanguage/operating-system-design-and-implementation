#include "ulib.h"

static int global_result = 0;  // 用于存储计算结果

// SIGUSR1 handler: increment the global result by 1
void sigusr1_handler(int signo) {
  printf("handle sigusr1\n");
  if (signo == SIGUSR1) {
    global_result++;
    global_result++;
  }
}

// SIGUSR2 handler: decrement the global result by 1
void sigusr2_handler(int signo) {
  printf("handle sigusr2\n");
  if (signo == SIGUSR2) {
    global_result--;
  }
}

// 子线程函数：通过信号来进行计算
int child_thread(void* arg) {
  int parent_pid = getpid();
  int signo = (size_t)arg % 2 == 0 ? SIGUSR1 : SIGUSR2;  // 偶数发送 SIGUSR1，奇数发送 SIGUSR2

  // printf("Child thread (TID: %d) sending signal %d to parent.\n", gettid(), signo);
  // 子线程向父进程发送信号
  kill(parent_pid, signo);
  return 0;
}

// 父进程协调多个子线程，等待计算结果
int signal_multi_thread_test() {
  void* old_sa = NULL;

  // 设置 SIGUSR1 和 SIGUSR2 信号处理函数
  sigaction(SIGUSR1, sigusr1_handler, &old_sa);
  sigaction(SIGUSR2, sigusr2_handler, &old_sa);

  int num_threads = 10;
  int threads[num_threads];  // 存储线程 ID
  void* stacks[num_threads];

  // 创建多个子线程
  for (int i = 0; i < num_threads; i++) {
    stacks[i] = malloc(4096);  // 为线程栈分配空间
    if (stacks[i] == NULL) {
      return -1;  // 内存分配失败
    }
    threads[i] = clone(child_thread, stacks[i], (void*)i);  // 创建线程
    join(threads[i], NULL);
  }

  // 预期计算结果：SIGUSR1 + 2 次，SIGUSR2 - 2 次，结果应为 0
  if (global_result == num_threads / 2) {
    return 0;  // 成功
  } else {
    return -1;  // 失败
  }
}

int main() {
  printf("Multi-thread signal test begins.\n");

  if (signal_multi_thread_test() == 0) {
    printf("Multi-thread signal test passed.\n");
  } else {
    printf("Multi-thread signal test failed.\n");
  }

  return 0;
}