#include "ulib.h"

// 线程执行的函数
int thread_function(void* arg) {
    sleep(5);  // 先等待父进程运行
    printf("Detached thread %d is running in process %d\n", gettid(), getpid());
    sleep(30);  // 模拟工作
    printf("Detached thread %d finished in process %d\n", gettid(), getpid());
    return 0;
}

// 测试1：主线程通过 return 退出
void test_return_exit() {
    printf("Running test_return_exit in process %d\n", getpid());
    
    // 为线程分配栈内存
    void *stack = malloc(4096);
    if (!stack) {
        printf("Memory allocation for stack failed.\n");
        exit(1);
    }

    // 创建一个新线程
    int tid = clone(thread_function, stack, NULL);
    if (tid == -1) {
        printf("clone failed.\n");
        free(stack);  // 在失败时释放栈
        exit(1);
    }

    // 分离线程
    if (detach(tid) != 0) {
        printf("detach failed.\n");
        free(stack);  // 在失败时释放栈内存
        exit(1);
    }

    // 主线程不等待分离的线程
    printf("Main thread (PID: %d): Not waiting for the detached thread\n", getpid());
    sleep(10);  // 模拟主线程的工作

    printf("Main thread (PID: %d): Finishing with return\n", getpid());
    return;  // 主线程通过 return 退出
}

// 测试2：主线程通过 thread_exit 退出
void test_thread_exit() {
    printf("Running test_thread_exit in process %d\n", getpid());
    
    // 为线程分配栈内存
    void *stack = malloc(4096);
    if (!stack) {
        printf("Memory allocation for stack failed.\n");
        exit(1);
    }

    // 创建一个新线程
    int tid = clone(thread_function, stack, NULL);
    if (tid == -1) {
        printf("clone failed.\n");
        free(stack);  // 在失败时释放栈
        exit(1);
    }

    // 分离线程
    if (detach(tid) != 0) {
        printf("detach failed.\n");
        free(stack);  // 在失败时释放栈内存
        exit(1);
    }

    // 主线程不等待分离的线程
    printf("Main thread (PID: %d): Not waiting for the detached thread\n", getpid());
    sleep(10);  // 模拟主线程的工作

    printf("Main thread (PID: %d): Finishing with thread_exit\n", getpid());

    thread_exit(0);  // 主线程通过 thread_exit 退出
}

int main() {
    printf("Starting combined test in main process (PID: %d)\n", getpid());

    // 创建两个子进程来分别运行两种测试逻辑
    int pid1 = fork();
    if (pid1 == 0) {
        // 子进程1，执行测试1
        test_return_exit();
        exit(0);
    }

    wait(NULL);

    int pid2 = fork();
    if (pid2 == 0) {
        // 子进程2，执行测试2
        test_thread_exit();
        exit(0);
    }

    wait(NULL);

    printf("Main process (PID: %d) finished\n", getpid());

    return 0;
}