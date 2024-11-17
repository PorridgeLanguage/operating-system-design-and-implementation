/*用例1：硬链接文件和源文件同时打开读写一致性
此测试用例不再使用O_APPEND，而是在读取和写入文件时手动管理文件偏移量。*/

#include "ulib.h"

#define FILENAME "testfile_consistency.txt"
#define LINKNAME "testfile_link_consistency.txt"
#define BUF_SIZE 1024

void test_hard_link_consistency() {
    int fd1, fd2;
    char buffer[BUF_SIZE];
    const char *initial_content = "Initial content.\n";
    const char *new_content = "New content added.\n";

    // Step 1: Create and write to the original file
    fd1 = open(FILENAME, O_WRONLY | O_CREATE | O_TRUNC);
    if (fd1 < 0) {
        printf("Error: Failed to open %s\n", FILENAME);
        return;
    }
    write(fd1, initial_content, strlen(initial_content));
    close(fd1);

    // Step 2: Create a hard link to the original file
    if (link(FILENAME, LINKNAME) < 0) {
        printf("Error: Failed to create link %s\n", LINKNAME);
        return;
    }

    // Step 3: Open both the original and the hard link files
    fd1 = open(FILENAME, O_WRONLY);
    fd2 = open(LINKNAME, O_RDONLY);
    if (fd1 < 0 || fd2 < 0) {
        printf("Error: Failed to open files.\n");
        return;
    }

    // Step 4: Write new content to the original file (append manually)
    lseek(fd1, 0, SEEK_END);  // Move to the end of the file
    write(fd1, new_content, strlen(new_content));
    close(fd1);

    // Step 5: Read from the hard link file
    size_t bytesRead = read(fd2, buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = '\0';  // Null-terminate the string

    // Step 6: Verify the content read from the hard link
    printf("Content read from %s after writing to %s:\n%s", LINKNAME, FILENAME, buffer);

    // Cleanup
    close(fd2);
    unlink(FILENAME);
    unlink(LINKNAME);
}

int main() {
    printf("Hard link consistency test begins.\n");
    test_hard_link_consistency();
    printf("Hard link consistency test ends.\n");
    return 0;
}