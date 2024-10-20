#include "ulib.h"

#define MEM_SIZE (64 * 1024 * 1024) // 64MB

int main() {
    printf("cowtest begins.\n");
    // Allocate a large memory region
    char *mem = (char *)malloc(MEM_SIZE);
    if (mem == NULL) {
        printf("Failed to allocate memory\n");
        return -1;
    }

    // Initialize memory content
    for (int i = 0; i < MEM_SIZE; ++i) {
        mem[i] = 'A'; // Initialize with 'A'
    }

    // Fork a child process
    int pid = fork();
    if (pid == -1) {
        printf("Failed to fork\n");
        free(mem);
        return -1;
    } else if (pid == 0) {
        // Child process
        // Modify part of the memory content
        for (int i = 0; i < MEM_SIZE / 2; ++i) {
            mem[i] = 'B'; // Modify the first half to 'B'
        }
        exit(0);
    } else {
        // Parent process
        // Wait for the child process to exit
        wait(NULL);

        // Check the memory content
        int diff = 0;
        for (int i = 0; i < MEM_SIZE / 2; ++i) {
            if (mem[i] != 'A') {
                diff = 1;
                break;
            }
        }

        if (diff) {
            printf("Parent: Memory content has been modified by child\n");
        } else {
            printf("Parent: Memory content remains unchanged\n");
        }
    }

    free(mem);
    printf("cowtest ends.\n");
    return 0;
}