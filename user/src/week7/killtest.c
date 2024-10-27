#include "ulib.h"

int self_kill_test() {
    printf("Process (PID: %d) is running and will try to kill itself...\n", getpid());

    sleep(2);  // 等待一段时间再发送信号

    // 发送 SIGTERM 给当前进程（即自身）
    if (kill(getpid()) == 0) {
        printf("This line should not be printed if kill() works.\n");
    } else {
        printf("kill failed.\n");
        exit(1);
    }

    // 进程应该在此之前被终止
    printf("This line should not be printed if kill() works.\n");

    return 0;
}

int kill_child_test() {
    int pid = fork();  // 创建子进程

    if (pid < 0) {
        // fork 失败
        printf("fork failed.\n");
        exit(1);
    } else if (pid == 0) {
        // 子进程
        printf("Child process (PID: %d) waiting for kill...\n", getpid());

        // 无限循环，保持子进程运行
        while (1) {
            sleep(1);
        }
    } else {
        // 父进程
        sleep(3);  // 等待子进程启动

        printf("Parent process (PID: %d) trying to kill child (PID: %d)\n", getpid(), pid);

        // 发送 SIGTERM 给子进程
        if (kill(pid) == 0) {
            printf("The child process (PID: %d) is killed\n", pid);
        } else {
            printf("kill failed.\n");
        }

        // 等待子进程终止
        int status;
        wait(&status);

        if (status == 0) {
            printf("Child process (PID: %d) exited with status %d\n", pid, status);
        } else {
            printf("Child process (PID: %d) killed by signal %d\n", pid, status);  // 预计输出此信息
        }
    }

    return 0;
}

int main() {
    printf("Main process (PID: %d) starts running.\n", getpid());

    // 创建子进程1执行自杀测试
    int pid1 = fork();
    if (pid1 == 0) {
        // 子进程1执行自杀测试
        self_kill_test();
        exit(0);
    }
    wait(NULL);

    // 创建子进程2执行父进程杀死子进程的测试
    int pid2 = fork();
    if (pid2 == 0) {
        // 子进程2执行父进程杀死子进程的测试
        kill_child_test();
        exit(0);
    }

    wait(NULL);

    printf("Main process (PID: %d) finished.\n", getpid());
    
    return 0;
}