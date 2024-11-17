#include "ulib.h"

void print_usage() {
    printf("Usage:\n");
    printf("  ln [source] [target]          # Create a hard link\n");
    printf("  ln -s [source] [target]       # Create a symbolic (soft) link\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage();
        return -1;
    }

    // Check if symbolic link flag (-s) is used
    int symbolic = 0;
    const char *src = NULL;
    const char *target = NULL;

    if (strcmp(argv[1], "-s") == 0) {
        if (argc != 4) {
            print_usage();
            return -1;
        }
        symbolic = 1; // Symbolic link
        src = argv[2];
        target = argv[3];
    } else {
        if (argc != 3) {
            print_usage();
            return -1;
        }
        src = argv[1];
        target = argv[2];
    }

    // Create the appropriate link
    int ret;
    if (symbolic) {
        ret = symlink(src, target);
        if (ret < 0) {
            printf("Error: Failed to create symbolic link from %s to %s\n", src, target);
            return -1;
        } else {
            // printf("Symbolic link created: %s -> %s\n", target, src);
        }
    } else {
        ret = link(src, target);
        if (ret < 0) {
            printf("Error: Failed to create hard link from %s to %s\n", src, target);
            return -1;
        } else {
            // printf("Hard link created: %s -> %s\n", target, src);
        }
    }

    return 0;
}