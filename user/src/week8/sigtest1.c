#include "ulib.h"

int stop_continue_test() {
    printf("Child process (PID: %d) is running...\n", getpid());

    // 子进程会在等待一段时间后退出，模拟有限时间的运行
    for (int i = 0; i < 10; i++) {
        sleep(1);  // 模拟每秒的工作
    }

    printf("Child process (PID: %d) is exiting after running.\n", getpid());
    return 0;
}

int main() {
    printf("Main process (PID: %d) starts running.\n", getpid());

    int pid = fork();  // 创建子进程

    if (pid < 0) {
        // fork 失败
        printf("fork failed.\n");
        exit(1);
    } else if (pid == 0) {
        // 子进程
        stop_continue_test();
        exit(0);
    } else {
        // 父进程
        sleep(2);  // 等待子进程启动并执行

        // 发送 SIGSTOP 给子进程
        if (kill(pid, SIGSTOP) == 0) {
            printf("Sent SIGSTOP to child (PID: %d).\n", pid);
        }

        sleep(3);  // 确保子进程处于停止状态

        // 发送 SIGCONT 给子进程
        if (kill(pid, SIGCONT) == 0) {
            printf("Sent SIGCONT to child (PID: %d).\n", pid);
        }

        // 等待子进程终止
        int status;
        wait(&status);

        // 评测子进程的状态
        if (status == 0) {
            printf("Child exited normally with status %d.\n", status);
        } else {
            printf("Child terminated abnormally.\n");
        }
    }

    printf("Main process (PID: %d) finished.\n", getpid());
    return 0;
}