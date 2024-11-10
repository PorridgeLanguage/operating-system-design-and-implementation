#include "ulib.h"  // 包含你操作系统的API定义

#define NUM_PROCESSES 4
#define NUM_THREADS 5
#define FILENAME "testfile.txt"
#define BUFFER_SIZE 100

int sem;
int mutex, cv;  // 添加信号量和条件变量

typedef struct monitor {
    int write_count;
    int expected_count;
} monitor_t;

monitor_t *monitor_ptr;

void clear_file() {
    int fd = open(FILENAME, O_WRONLY | O_CREATE);
    if (fd >= 0) {
        close(fd);  // 创建文件并清空内容
    }
}

int count_lines(const char *buf) {
    int count = 0;
    const char *ptr = buf;

    while (*ptr) {
        if (*ptr == '\n') {
            count++;
        }
        ptr++;
    }

    // 如果buf非空且最后一个字符不是换行符，增加一行
    if (count > 0 && buf[strlen(buf) - 1] != '\n') {
        count++;
    }

    return count;
}

int thread_write(void* arg) {
    int fd;
    int thread_id = (int)(long)arg;  // 获取线程ID
    char buf[BUFFER_SIZE];

    // 写入线程ID到文件
    sprintf(buf, "%d\n", thread_id);

    // 加锁，开始写入
    P(sem);
    fd = open(FILENAME, O_WRONLY | O_CREATE);
    if (fd < 0) {
        V(sem);
        return 1;
    }

    // 写入文件    
    lseek(fd, 0, SEEK_END);
    write(fd, buf, strlen(buf));
    close(fd);
    
    // 更新写入计数并检查条件
    P(mutex);
    monitor_ptr->write_count++;
    cv_sigall(cv);
    V(mutex);
    V(sem);

    return 0;
}

int thread_read(void* arg) {
    int fd;
    char buf[BUFFER_SIZE];
    int expected_count = NUM_THREADS * NUM_PROCESSES;
    int actual_count = 0;

    // 等待所有写线程完成
    P(mutex);
    while (monitor_ptr->write_count < expected_count) {
        cv_wait(cv, mutex);
        P(mutex);
    }
    cv_sigall(cv);
    V(mutex);

    // 读取文件内容
    fd = open(FILENAME, O_RDONLY);
    if (fd < 0) {
        return 1;
    }

    // 读取文件并计算行数
    read(fd, buf, BUFFER_SIZE - 1);
    actual_count = count_lines(buf);

    close(fd);

    // 验证实际读取的行数是否与预期相同
    if (actual_count == expected_count) {
        // printf("Process %d: Content matches expected. (%d lines)\n", getpid(), actual_count);
    } else {
        printf("Process %d: Content does NOT match expected. Expected: %d, Got: %d\n", getpid(), expected_count, actual_count);
    }

    return 0;
}

void run_threads() {
    void* stacks[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    // monitor_ptr->expected_count = NUM_THREADS;  // 设置期望的写入计数
    
    // 创建多个写线程
    for (int i = 0; i < NUM_THREADS; ++i) {
        stacks[i] = malloc(4096);  // 分配线程栈
        thread_ids[i] = clone(thread_write, stacks[i]+4096, (void*)(long)i);
        if (thread_ids[i] == -1) {
            exit(1);
        }
    }

    // 等待写线程完成
    for (int i = 0; i < NUM_THREADS; ++i) {
        join(thread_ids[i], NULL);
        free(stacks[i]);  // 释放线程栈
    }

    // 创建多个读线程
    for (int i = 0; i < NUM_THREADS; ++i) {
        stacks[i] = malloc(4096);  // 再次分配线程栈
        thread_ids[i] = clone(thread_read, stacks[i]+4096, NULL);
        if (thread_ids[i] == -1) {
            free(stacks[i]);
            exit(1);
        }
    }

    // 等待读线程完成
    for (int i = 0; i < NUM_THREADS; ++i) {
        join(thread_ids[i], NULL);
        free(stacks[i]);  // 释放线程栈
    }
}

int main() {
    printf("Multi-thread file test begins.\n");
    clear_file();  // 清空文件内容以开始新的测试
    int pids[NUM_PROCESSES];

    sem = sem_open(1);
    mutex = sem_open(1);
    cv = cv_open();

    monitor_ptr = mmap();
    monitor_ptr->expected_count = NUM_THREADS * NUM_PROCESSES;  // 设置期望的写入计数
    monitor_ptr->write_count = 0;

    // 创建多个进程
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        pids[i] = fork();

        if (pids[i] == -1) {
            exit(1);
        }

        if (pids[i] == 0) {  // 子进程
            run_threads();  // 每个进程创建多个线程
            sem_close(sem);
            exit(0);
        }
    }

    // 父进程等待所有子进程完成
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        wait(NULL);
    }

    printf("All processes completed successfully, test passed.\n");
    return 0;
}