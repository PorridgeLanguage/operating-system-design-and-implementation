#include "ulib.h"

int thread_function(void* arg) {
    printf("Thread %d is running\n", gettid());
    sleep(30);  // Simulate some work
    printf("Thread %d finished\n", gettid());
    return 0;
}

int main() {    
    // Create a new thread
    void *stack = malloc(4096);
    int tid = clone(thread_function, stack, NULL);
    

    if (tid == -1) {
        printf("clone failed.\n");
        free(stack);
        return 1;
    }

    // Wait for the thread to finish
    if (join(tid, NULL) != 0) {
        printf("join failed.\n");
        return 1;
    }

    printf("Main thread: The thread has finished execution\n");
    free(stack);
    return 0;
}