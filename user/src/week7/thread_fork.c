#include "ulib.h"
#include <stdbool.h>

#define THREAD_COUNT 2

// Data structure to verify thread execution
typedef struct {
    bool thread_fork_within_thread_done;
    bool main_thread_fork_done;
} VerificationData;

// Global variable to store verification data
VerificationData *verification_data;

// semaphore
int sem;

// 线程函数：在线程内调用 fork 的测试用例
int thread_fork_within_thread(void* arg) {
    verification_data->thread_fork_within_thread_done = true; // Mark thread as done
    
    // int ptid = gettid();
    // int ppid = getpid();

    // 在线程中调用 fork
    int pid = fork();

    if (pid < 0) {
        exit(-1);
    } else if (pid == 0) {
        // 子进程
        sleep(10);  // 模拟处理
        V(sem);  // 通知信号量
        exit(0);
    } else {
        // 父进程的线程继续运行
        int status;
        wait(&status);  // 等待子进程结束
        if (status != 0) {
            exit(-1);
        }
        V(sem);  // 通知信号量
    }

    return 0;
}

// 线程函数：主线程调用 fork，线程继续运行的测试用例
int thread_function(void* arg) {
    verification_data->main_thread_fork_done = true; // Mark thread as done
    sleep(30);  // 模拟处理
    V(sem);  // 通知信号量
    return 0;
}

int main() {
    printf("thread_fork begins.\n");

    int pid;
    verification_data = (VerificationData *)mmap(); // Use shared memory for communication across processes
    *verification_data = (VerificationData){false, false}; // Initialize verification data

    // 初始化信号量，初始值为 0
    sem = sem_open(0);

    // 子进程 1：测试线程内部 fork
    pid = fork();
    if (pid < 0) {
        return 1; // fork failed
    } else if (pid == 0) {
        void *stack1 = malloc(4096);
        int tid = clone(thread_fork_within_thread, stack1 + 4096, NULL);
        if (tid == -1) {
            printf("thread_fork failed: clone failed.\n");
            free(stack1);
            exit(-1);
        }
        P(sem);  // 等待信号量，确保子进程和线程同步
        P(sem);  // 等待信号量，确保子进程结束
        free(stack1);
        exit(0);
    }
    wait(NULL);

    // 子进程 2：测试主线程 fork，线程继续运行
    pid = fork();
    if (pid < 0) {
        return 1; // fork failed
    } else if (pid == 0) {
        void *stack2 = malloc(4096);
        int tid = clone(thread_function, stack2 + 4096, NULL);
        if (tid == -1) {
            printf("thread_fork failed: clone failed.\n");
            free(stack2);
            exit(-1);
        }

        // 在创建线程后立即调用 fork
        int child_pid = fork();
        if (child_pid < 0) {
            printf("thread_fork failed: fork failed.\n");
            free(stack2);
            exit(-1);
        } else if (child_pid == 0) {
            sleep(10);  // 模拟任务处理
            exit(0);
        } else {
            int status;
            wait(&status);  // 等待子进程结束
            if (status != 0) {
                printf("thread_fork failed: wait failed.\n");
                free(stack2);
                exit(-1);
            }
            P(sem);  // 等待线程完成
            free(stack2);
            exit(0);
        }
    }
    wait(NULL);

    // 验证子进程中的线程是否按预期执行
    if (!verification_data->thread_fork_within_thread_done || !verification_data->main_thread_fork_done) {
        printf("thread_fork failed: thread did not execute as expected.\n");
        return 1; // Indicate failure if any thread did not execute as expected
    }

    // 销毁信号量
    sem_close(sem);

    printf("thread_fork passed.\n");
    return 0; // 程序正常结束
}