/*用例1：测试软链接循环
创建两个软链接，它们相互指向对方，检查是否能够正确检测到并处理循环引用的问题。*/

#include "ulib.h"

#define SYMLINK1 "symlink1.txt"
#define SYMLINK2 "symlink2.txt"

void test_symlink_loop() {
    // Create two symlinks that point to each other
    if (symlink(SYMLINK2, SYMLINK1) < 0) {
        printf("Failed to create symlink %s\n", SYMLINK1);
        exit(1);
    }
    
    if (symlink(SYMLINK1, SYMLINK2) < 0) {
        printf("Failed to create symlink %s\n", SYMLINK2);
        exit(1);
    }
    
    // Try to read from the symlink, which should fail due to the loop
    int fd = open(SYMLINK1, O_RDONLY);
    if (fd < 0) {
        printf("Error: Detected a symlink loop when trying to open %s\n", SYMLINK1);
    } else {
        printf("Unexpected: Managed to open %s\n", SYMLINK1);
        close(fd);
    }

    // Clean up: remove the symlinks
    unlink(SYMLINK1);
    unlink(SYMLINK2);
}

int main() {
    printf("symlink loop test begins.\n");
    test_symlink_loop();
    printf("symlink loop test ends.\n");
    return 0;
}