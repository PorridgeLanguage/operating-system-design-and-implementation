/*测试在源文件和硬链接文件都存在的情况下，删除源文件后硬链接文件的行为。 */

#include "ulib.h"

#define FILENAME "testfile_delete.txt"
#define LINKNAME "testfile_link_delete.txt"
#define BUF_SIZE 1024

void test_hard_link_delete() {
    int fd;
    char buffer[BUF_SIZE];
    const char *content = "This content will be used to test deletion.\n";

    // Step 1: Create and write to the original file
    fd = open(FILENAME, O_WRONLY | O_CREATE | O_TRUNC);
    write(fd, content, strlen(content));
    close(fd);

    // Step 2: Create a hard link to the original file
    if (link(FILENAME, LINKNAME) < 0) {
        printf("Error: Failed to create link %s\n", LINKNAME);
        return;
    }

    // Step 3: Delete the original file
    if (unlink(FILENAME) < 0) {
        printf("Error: Failed to unlink %s\n", FILENAME);
        return;
    }

    // Step 4: Read from the hard link file
    fd = open(LINKNAME, O_RDONLY);
    if (fd < 0) {
        printf("Error: Failed to open %s\n", LINKNAME);
        return;
    }
    size_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = '\0';  // Null-terminate the string
    close(fd);

    // Step 5: Verify the content read from the hard link
    printf("Content read from %s after %s was deleted:\n%s", LINKNAME, FILENAME, buffer);

    // Cleanup
    unlink(LINKNAME);
}

int main() {
    printf("Hard link delete test begins.\n");
    test_hard_link_delete();
    printf("Hard link delete test ends.\n");
    return 0;
}