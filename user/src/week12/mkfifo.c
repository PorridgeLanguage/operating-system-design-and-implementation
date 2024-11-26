#include "ulib.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: mkfifo <filename>\n");
        return 1;
    }

    // 调用 mkfifo 函数创建有名管道
    if (mkfifo(argv[1], O_CREATE | O_RDONLY) < 0) {
        printf("Error: failed to create FIFO %s\n", argv[1]);
        return 1;
    }

    // printf("FIFO %s created successfully.\n", argv[1]);
    return 0;
}