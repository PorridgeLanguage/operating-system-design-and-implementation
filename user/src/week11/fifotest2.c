#include "ulib.h"

#define FIFO_PATH "testfifo2"
#define PIPE_SIZE 128
#define LARGE_WRITE_SIZE 1024

int main() {
    printf("fifo large write test begins.\n");
    char write_msg[LARGE_WRITE_SIZE]; // 超过FIFO容量的消息
    char read_msg[LARGE_WRITE_SIZE];
    int nbytes;

    // 填充写消息
    for (int i = 0; i < sizeof(write_msg); i++) {
        write_msg[i] = 'A' + (i % 26);
    }

    int tmp;
    if((tmp = open(FIFO_PATH, O_RDONLY)) > 0){
        unlink(FIFO_PATH);
        close(tmp);
    }

    // 创建FIFO文件
    if (mkfifo(FIFO_PATH, O_CREATE|O_RDWR) < 0) {
        printf("create fifo failed.\n");
        return -1;
    }

    int pid = fork();

    if (pid < 0) {
        // 创建子进程失败
        printf("fork fails.\n");
        return -1;
    } else if (pid == 0) {
        // 子进程，负责读取FIFO中的数据
        int fd_read = open(FIFO_PATH, O_RDONLY);
        if (fd_read < 0) {
            printf("open fifo read end failed.\n");
            return -1;
        }

        int total_read = 0;
        // 从FIFO读取数据
        while ((nbytes = read(fd_read, read_msg + total_read, sizeof(read_msg) - total_read)) > 0) {
            total_read += nbytes;
            if(total_read == LARGE_WRITE_SIZE) break;
        }

        // 打印读取的数据长度
        if (total_read != LARGE_WRITE_SIZE) {
            printf("fifo large write failed: expect to read %d bytes but got %d instead.\n", LARGE_WRITE_SIZE, total_read);
            return 1;
        }

        close(fd_read); // 关闭读端
        return 0;
    } else {
        // 父进程，负责向FIFO写入数据
        int fd_write = open(FIFO_PATH, O_WRONLY);
        if (fd_write < 0) {
            printf("open fifo write end failed.\n");
            return -1;
        }

        // 向FIFO写入数据
        write(fd_write, write_msg, sizeof(write_msg));

        close(fd_write); // 关闭写端

        // 等待子进程完成
        int status;
        wait(&status);
        if (!status) printf("fifo large write test passed.\n");
    }

    // 删除FIFO文件
    unlink(FIFO_PATH);
    return 0;
}