#include "ulib.h"

int thread_exec_test() {
    printf("Thread (TID: %d) in process (PID: %d) running...\n", gettid(), getpid());
    
    sleep(10);  // 模拟线程的工作

    // 在线程中执行 exec 替换整个进程的地址空间
    char *args[] = {"echo", "good", NULL};
    exec("echo", args);

    // 以下代码不会执行
    printf("exec failed in thread.\n");
    return -1;
}

int main_exec_test() {
    printf("Main thread (TID: %d) in process (PID: %d) running...\n", gettid(), getpid());
    
    sleep(10);  // 主线程等待一段时间

    // 在主线程中执行 exec 替换整个进程的地址空间
    char *args[] = {"echo", "good", NULL};
    exec("echo", args);

    // 以下代码不会执行
    printf("exec failed in main.\n");
    return -1;
}

int main() {
    printf("Main process (PID: %d) started.\n", getpid());

    // 创建子进程1：测试线程调用 exec
    int pid1 = fork();
    if (pid1 == 0) {
        // 子进程1中测试线程调用 exec
        void *stack = malloc(4096);
        if (clone(thread_exec_test, stack, NULL) == -1) {
            printf("clone failed in thread_exec_test.\n");
            free(stack);
            exit(1);
        }

        sleep(50);  // 让主进程等待足够长的时间，以保证线程有机会执行 exec
        exit(0);
    }

    wait(NULL);

    // 创建子进程2：测试主线程调用 exec
    int pid2 = fork();
    if (pid2 == 0) {
        // 子进程2中测试主线程调用 exec
        main_exec_test();
        exit(0);
    }

    wait(NULL);

    printf("Main process (PID: %d) finished.\n", getpid());
    
    return 0;
}