#include "ulib.h"

char buf[4096];

void
cat(int fd, int type)
{
    int n;

    // 读取文件内容并输出到 stdout
    while((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(STD_OUT, buf, n) != n) {
            fprintf(2, "cat: write error\n");
            exit(1);
        }
        if(type == TYPE_FIFO) break;
    }
    if(n < 0) {
        fprintf(2, "cat: read error\n");
        exit(1);
    }
}

int
main(int argc, char *argv[])
{
    int fd, i;
    struct stat st;

    if(argc <= 1){
        // get file info, check whether it is fifo
        if(fstat(STD_IN, &st) < 0) {
            fprintf(STD_ERR, "cat: cannot stat STD_IN in REDIR\n");
            return 1;
        }

        if(!st.size) return 0;
        
        // call cat and output 
        cat(STD_IN, st.type);
        exit(0);
    }

    for(i = 1; i < argc; i++) {
        // open file
        if((fd = open(argv[i], 0)) < 0) {
            fprintf(2, "cat: cannot open %s\n", argv[i]);
            exit(1);
        }

        // get file info, check whether it is fifo
        if(fstat(fd, &st) < 0) {
            fprintf(2, "cat: cannot stat %s\n", argv[i]);
            close(fd);
            continue;
        }

        if(!st.size) return 0;
        
        // call cat and output 
        cat(fd, st.type);
        close(fd);
    }
    exit(0);
}