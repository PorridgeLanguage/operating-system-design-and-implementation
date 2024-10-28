#include "ulib.h"

static int usr1_count = 0;
static int usr2_count = 0;

// SIGUSR1 handler: increment usr1_count
void sigusr1_handler(int signo) {
    if (signo == SIGUSR1) {
        usr1_count++;
    }
}

// SIGUSR2 handler: increment usr2_count
void sigusr2_handler(int signo) {
    if (signo == SIGUSR2) {
        usr2_count++;
    }
}

// Test signal handling with blocking/unblocking
int signal_test() {
    void *old_sa = NULL;
    int mask = 0;

    // Set up SIGUSR1 handler
    sigaction(SIGUSR1, sigusr1_handler, NULL);

    // Set up SIGUSR2 handler
    sigaction(SIGUSR2, sigusr2_handler, &old_sa);

    // Block SIGUSR1 and SIGUSR2
    mask |= (1 << SIGUSR1) | (1 << SIGUSR2);
    sigprocmask(SIG_BLOCK, mask, NULL);

    // Send SIGUSR1 and SIGUSR2
    kill(getpid(), SIGUSR1);
    kill(getpid(), SIGUSR2);
    sleep(2);
    
    // At this point, usr1_count and usr2_count should still be 0 because both signals are blocked
    if (usr1_count != 0 || usr2_count != 0) {
        return -1;  // Failure: signals were not blocked properly
    }

    // Unblock SIGUSR1, leave SIGUSR2 blocked
    mask &= ~(1 << SIGUSR1);
    sigprocmask(SIG_SETMASK, mask, NULL);
    sleep(2);

   // SIGUSR1 should have been handled, but SIGUSR2 should still be blocked
    if (usr1_count != 1 || usr2_count != 0) {
        return -1;  // Failure: SIGUSR1 was not handled or SIGUSR2 was not blocked correctly
    }

    // Unblock SIGUSR2
    mask &= ~(1 << SIGUSR2);
    sigprocmask(SIG_SETMASK, mask, NULL);
    sleep(2);

    // Now both signals should have been handled
    if (usr1_count != 1 || usr2_count != 1) {
        return -1;  // Failure: SIGUSR2 was not handled correctly
    }

    // If all checks passed
    return 0;
}

int main() {
    printf("sigaction begins.\n");
    // Run the test and check the result
    if (signal_test() == 0) {
        // Success: All internal checks passed, minimal output
        printf("sigaction test passed.\n");
    } else {
        // Failure: Something went wrong, minimal output
        printf("sigaction test failed.\n");
    }
    return 0;
}