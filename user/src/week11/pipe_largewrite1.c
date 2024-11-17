#include "ulib.h"

// 测试向管道写入超过其容量的数据。确保管道能够正确处理超量数据，并且在写入时阻塞。

#define PIPE_SIZE 128
#define LARGE_WRITE_SIZE 1024

int main() {
    printf("pipe large write test 1 begins.\n");
    int fd[2];
    char write_msg[LARGE_WRITE_SIZE]; // 超过管道容量的消息
    char read_msg[LARGE_WRITE_SIZE];
    int nbytes;

    // 填充写消息
    for (int i = 0; i < sizeof(write_msg); i++) {
        write_msg[i] = 'A' + (i % 26);
    }

    // 创建管道
    if (pipe(fd) == -1) {
        printf("create pipe failed.\n");
        return -1;
    }

    int pid = fork();

    if (pid < 0) {
        // 创建子进程失败
        printf("fork fails.\n");
        return -1;
    } else if (pid == 0) {
        // 子进程
        close(fd[1]); // 关闭写端

        // 从管道读取数据

        int total_read = 0;
        while ((nbytes = read(fd[0], read_msg + total_read, sizeof(read_msg) - total_read)) > 0) {
            total_read += nbytes;
        }

        // 打印读取的数据长度
        if(total_read != LARGE_WRITE_SIZE){
            printf("pipe large write 1 failed: expect to read %d bytes but get %d instead.\n", LARGE_WRITE_SIZE, total_read);
            return 1;
        }

        close(fd[0]); // 关闭读端
        return 0;
    } else {
        // 父进程
        close(fd[0]); // 关闭读端

        // 向管道写入数据
        write(fd[1], write_msg, sizeof(write_msg));

        close(fd[1]); // 关闭写端

        // 等待子进程完成
        int status;
        wait(&status);
        if(!status) printf("pipe large write test 1 passed.\n");
    }
    return 0;
}