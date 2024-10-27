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

// 线程函数
int thread_function(void* arg) {
    int thread_id = *(int*)arg;
    verification_data->threads_started[thread_id] = true; // Mark thread as started
    sleep(20); // 先睡一会儿，等主线程先结束
    verification_data->threads_ended[thread_id] = true; // Mark thread as ended
    return 0;
}

int main() {
    printf("thread_return begins.\n");

    int pid, wpid;
    int status;

    // 初始化验证数据
    verification_data = (VerificationData *)mmap();
    *verification_data = (VerificationData){{false, false}, {false, false}}; 

    // 创建一个子进程
    pid = fork();

    if (pid < 0) {
        // fork 失败
        printf("thread_return failed: fork failed.\n");
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
        
        sleep(5); // Give some time for thread to begin

        return 0; // 子进程先于线程正常退出
    } else {
        // 父进程代码
        wpid = wait(&status);

        if (wpid == -1) {
            printf("thread_return failed: wait failed.\n");
            return 1; // wait 失败
        }

        // 验证所有线程是否按预期结束
        for (int i = 0; i < THREAD_COUNT; i++) {
            if (!verification_data->threads_started[i] || verification_data->threads_ended[i]) {
                printf("thread_return failed: some thread did not start or end as expected.\n");
                return 1; // 线程启动或结束不符合预期
            }
        }

        if (status != 0) {
            printf("thread_return failed: child process abnormally exits.\n");
            return 1; // 子进程异常退出
        }
    }

    printf("thread_return passed.\n");
    return 0; // 程序正常结束
}