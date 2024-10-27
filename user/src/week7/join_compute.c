#include "ulib.h"

#define NUM_THREADS 4
#define ARRAY_SIZE 16

// Function for each thread to compute partial sum
int compute_partial_sum(void* arg) {
    int* array = (int*)arg;
    int sum = 0;

    for (int i = 0; i < ARRAY_SIZE / NUM_THREADS; i++) {
        sum += array[i];
    }

    int* result = (int*)malloc(sizeof(int));  // Allocate space to store the result
    *result = sum;

    return (int)result;  // Return the result
}

int main() {
    int threads[NUM_THREADS];
    void *stacks[NUM_THREADS];
    int array[ARRAY_SIZE];
    int total_sum = 0;

    // Initialize the array with values
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = i + 1;  // For example: array = {1, 2, 3, ..., 16}
    }

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        // Each thread gets a different part of the array to compute
        stacks[i] = malloc(4096);
        threads[i] = clone(compute_partial_sum, stacks[i], (void*)(array + i * (ARRAY_SIZE / NUM_THREADS)));
        if (threads[i] == -1) {
            printf("pthread_create failed.\n");
            free(stacks[i]);  // 在失败时释放栈
            return 1;
        }
    }

    // Collect results from threads
    for (int i = 0; i < NUM_THREADS; i++) {
        void* result;
        if (join(threads[i], &result) != 0) {
            printf("join failed.\n");
            return 1;
        }

        // Add the partial sum returned by the thread to the total sum
        total_sum += *(int*)result;
        free(result);  // Free the memory allocated in the thread
        free(stacks[i]);
    }

    printf("Total sum calculated by threads: %d\n", total_sum);  // Should be 136 (sum of 1 to 16)

    return 0;
}