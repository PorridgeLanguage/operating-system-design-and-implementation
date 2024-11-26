#ifndef __FILE_H__
#define __FILE_H__

#include <stdint.h>
#include "dev.h"
#include "fs.h"
#include "sem.h"

#define PIPE_SIZE 128

typedef struct pipe {
  char buffer[PIPE_SIZE];  // the pipe buffer
  int read_pos;            // next position to read from
  int write_pos;           // next position to write in
  int read_open;           // whether read side is open
  int write_open;          // whether write side is open
  int full;                // record the readable byte number
  int empty;               // record the empty position number
  sem_t mutex;             // mutex for cv, unnecessary in our setting
  sem_t cv_buf;            // cv for operate on buffer, unnecessary in our setting
  int no;                  // for fifo consistency
} pipe_t;

typedef struct file {
  int type;
  int ref;
  int readable, writable;

  // for normal file
  inode_t* inode;
  uint32_t offset;

  // for dev file
  dev_t* dev_op;

  // for pipe & fifo
  pipe_t* pipe;
} file_t;

file_t* fopen(const char* path, int mode, int depth);
int fread(file_t* file, void* buf, uint32_t size);
int fwrite(file_t* file, const void* buf, uint32_t size);
uint32_t fseek(file_t* file, uint32_t off, int whence);
file_t* fdup(file_t* file);
void fclose(file_t* file);
int flink(const char* oldpath, const char* newpath);
int fsymlink(const char* oldpath, const char* newpath);

int pipe_open(file_t* pipe_files[2]);
int pipe_read(file_t* file, void* buf, uint32_t size);
int pipe_write(file_t* file, const void* buf, uint32_t size);
void pipe_close(pipe_t* pipe);

file_t* mkfifo(const char* path, int mode);
void rmfifo(int no);

#endif
