#include "ulib.h"

#define FIFO_PATH "./myfifo_speed3"
#define BUFFER_SIZE 50
#define MESSAGE "Message from writer process! No: %d"

void writer_process() {
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        printf("Failed to open FIFO for writing.\n");
        exit(1);
    }

    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    for (int i = 0; i < 10; i++) {
        // 为每条消息添加编号
        sprintf(message, MESSAGE, i);
        write(fd, message, strlen(message)+1); // +1 to include null terminator
    }
    close(fd);
    printf("Writer sent all messages successfully.\n");
}

void reader_process() {
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        printf("Failed to open FIFO for reading.\n");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    for (int i = 0; i < 10; i++) {
        sleep(2); // 模拟读取速度较慢
        read(fd, buffer, strlen(MESSAGE)); // 读取消息

        // 验证消息是否按顺序接收
        char expected_message[BUFFER_SIZE];
        sprintf(expected_message, MESSAGE, i);

        if (strcmp(buffer, expected_message) != 0) {
            printf("Reader received wrong message. Expected: \"%s\", but got: \"%s\"\n", expected_message, buffer);
            exit(1);
        }
    }
    close(fd);
    printf("Reader received all messages in correct order.\n");
}

int main() {
    printf("FIFO speed test begins.\n");

    // 创建FIFO
    if (mkfifo(FIFO_PATH, O_CREATE | O_RDONLY) == -1) {
        printf("Failed to create FIFO.\n");
        exit(1);
    }

    int writer_pid = fork();
    if (writer_pid == -1) {
        printf("Failed to fork writer process.\n");
        exit(1);
    } else if (writer_pid == 0) {
        // Writer 进程
        writer_process();
        exit(0);
    }

    int reader_pid = fork();
    if (reader_pid == -1) {
        printf("Failed to fork reader process.\n");
        exit(1);
    } else if (reader_pid == 0) {
        // Reader 进程
        reader_process();
        exit(0);
    }

    // 等待两个子进程结束
    wait(NULL);
    wait(NULL);

    // 清理FIFO文件
    unlink(FIFO_PATH);

    printf("FIFO speed test ends.\n");
    return 0;
}