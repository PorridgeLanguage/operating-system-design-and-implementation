#include "ulib.h"

// 测试多次读写操作。确保管道能够正确处理多次读写操作，并且数据不丢失。

int main() {
    printf("pipe multiple read/write test begins.\n");
    int fd[2];
    char write_msg1[] = "First message";
    char write_msg2[] = "Second message";
    char read_msg[128];
    int nbytes;

    // 创建管道
    if (pipe(fd) == -1) {
        printf("create pipe failed.\n");
        return -1;
    }

    int pid = fork();

    if (pid < 0) {
        // 创建子进程失败
        printf("fork fails.");
        return -1;
    } else if (pid == 0) {
        // 子进程
        close(fd[1]); // 关闭写端

        // 从管道读取数据
        nbytes = read(fd[0], read_msg, sizeof(read_msg));
        if (nbytes < 0) {
            printf("Child read from pipe failed.\n");
            return 1;
        }

        read_msg[nbytes] = '\0';
        if(strcmp(read_msg, write_msg1)){
            printf("Get wrong first message: expet %s but get %s", write_msg1, read_msg);
            return 1;
        }

        // 再次读取数据
        nbytes = read(fd[0], read_msg, sizeof(read_msg));
        if (nbytes < 0) {
            printf("Child read from pipe failed.\n");
            return 1;
        }

        read_msg[nbytes] = '\0';
        if(strcmp(read_msg, write_msg2)){
            printf("Get wrong second message: expet %s but get %s", write_msg2, read_msg);
            return 1;
        }

        close(fd[0]); // 关闭读端
    } else {
        // 父进程
        close(fd[0]); // 关闭读端

        // 向管道写入第一条消息
        if (write(fd[1], write_msg1, strlen(write_msg1)) != strlen(write_msg1)) {
            printf("Parent write to pipe failed.\n");
            return -1;
        }

        sleep(20); // Give some time for child process to get the message

        // 向管道写入第二条消息
        if (write(fd[1], write_msg2, strlen(write_msg2)) != strlen(write_msg2)) {
            printf("Parent write to pipe failed.\n");
            return -1;
        }

        close(fd[1]); // 关闭写端

        // 等待子进程完成
        int status;
        wait(&status);
        if(!status) printf("pipe multiple read/write test passed.\n");
    }
    return 0;
}