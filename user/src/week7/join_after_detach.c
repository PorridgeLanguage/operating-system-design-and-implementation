#include "ulib.h"

int thread_function(void* arg) {
    printf("Thread running...\n");
    sleep(30);
    return 0;  // 退出线程
}

int main() {    
    // 创建线程
    void *stack = malloc(4096);
    int tid = clone(thread_function, stack, NULL);
    if(tid == -1){
        printf("clone failed.\n");
        free(stack);
        exit(1);
    }

    // 分离线程
    detach(tid);
    sleep(10);

    // 尝试 join 已经被 detach 的线程
    int result = join(tid, NULL);
    // printf("%d %d\n", result, ESRCH);
    
    if (result != 0) {
        printf("Error: Cannot join a detached thread (ESRCH).\n");
    } else {
        printf("Thread joined successfully.\n");
    }

    return 0;
}