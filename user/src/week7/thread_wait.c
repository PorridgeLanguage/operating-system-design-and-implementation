#include "ulib.h"
#include <stdbool.h>

#define THREAD_COUNT 2

// Data structure to verify thread execution
typedef struct {
    bool threads_started[THREAD_COUNT];
    bool threads_ended[THREAD_COUNT];
} VerificationData;

// Global variable to store verification data
VerificationData *verification_data;

// 子进程中执行的线程函数
int thread_function(void *arg) {
    int thread_id = *(int *)arg;
    verification_data->threads_started[thread_id] = true; // Mark thread as started
    sleep(5); // 模拟线程的处理任务
    verification_data->threads_ended[thread_id] = true; // Mark thread as ended
    return 0;
}

// 父进程中调用 wait 的线程函数
int thread_wait_function(void *arg) {
    int *result = (int *)arg;
    int status;
    int wpid = wait(&status);
    if (wpid == -1) {
        *result = -1;
        return -1; // Indicate wait failure
    }

    // Verify that all threads in child process have ended
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (!verification_data->threads_ended[i]) {
            *result = -1;
            return -1; // Indicate failure if any thread has not ended
        }
    }
    *result = 0;
    return 0; // Indicate successful wait
}

int main() {
    printf("thread_wait begins.\n");

    int pid;
    verification_data = (VerificationData *)mmap(); // Use shared memory for communication across processes.s
    *verification_data = (VerificationData){{false, false}, {false, false}}; // Initialize verification data

    // 创建一个子进程
    pid = fork();

    if (pid < 0) {
        // fork 失败
        return 1;
    } else if (pid == 0) {
        // 子进程代码
        int thread_ids[THREAD_COUNT];
        void *stacks[THREAD_COUNT];

        // 创建两个线程
        for (int i = 0; i < THREAD_COUNT; i++) {
            thread_ids[i] = i;
            stacks[i] = malloc(4096);
            clone(thread_function, stacks[i] + 4096, (void *)&thread_ids[i]);
        }
        
        thread_exit(0); // 子进程及其线程正常退出
    } else {
        // 父进程代码
        void *stack = malloc(4096);
        int wait_result = -1;
        clone(thread_wait_function, stack + 4096, (void *)&wait_result);

        sleep(50); // Give some time for thread to operate

        if (wait_result != 0) {
            // 验证失败
            printf("thread_wait failed: wait_result != 0.\n");
            return 1;
        }

        // 验证子进程中的线程是否按预期结束
        for (int i = 0; i < THREAD_COUNT; i++) {
            if (!verification_data->threads_started[i] || !verification_data->threads_ended[i]) {
                printf("thread_wait failed: some thread did not start or end as expected.\n");
                return 1; // Indicate failure if any thread did not start or end
            }
        }
    }

    printf("thread_wait passed.\n");
    return 0;
}