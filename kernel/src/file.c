#include "file.h"
#include "klib.h"
#include "proc.h"
#include "vme.h"

#define TOTAL_FILE 128

file_t files[TOTAL_FILE];

static file_t* falloc() {
  // Lab3-1: find a file whose ref==0, init it, inc ref and return it, return NULL if none
  for (int i = 0; i < TOTAL_FILE; i++) {
    if (files[i].ref == 0) {
      files[i].type = TYPE_NONE;
      files[i].ref++;
      return &files[i];
    }
  }
  return NULL;
}

file_t* fopen(const char* path, int mode, int depth) {
  if (depth > 40) {
    return NULL;
  }
  file_t* fp = falloc();
  inode_t* ip = NULL;
  if (!fp)
    goto bad;
  // TODO: Lab3-2, determine type according to mode
  // iopen in Lab3-2: if file exist, open and return it
  //       if file not exist and type==TYPE_NONE, return NULL
  //       if file not exist and type!=TYPE_NONE, create the file as type
  // you can ignore this in Lab3-1
  int open_type = 114514;
  if (!(mode & O_CREATE)) {
    open_type = TYPE_NONE;
  } else {
    if (mode & O_DIR) {
      open_type = TYPE_DIR;
    } else {
      open_type = TYPE_FILE;
    }
  }
  ip = iopen(path, open_type);
  if (!ip)
    goto bad;
  int type = itype(ip);
  if (type == TYPE_FILE || type == TYPE_DIR) {
    // TODO: Lab3-2, if type is not DIR, go bad if mode&O_DIR
    if (type != TYPE_DIR && mode & O_DIR) {
      goto bad;
    }
    // TODO: Lab3-2, if type is DIR, go bad if mode WRITE or TRUNC
    if (type == TYPE_DIR && (mode & O_WRONLY || mode & O_RDWR || mode & O_TRUNC)) {
      goto bad;
    }
    // TODO: Lab3-2, if mode&O_TRUNC, trunc the file
    if (type == TYPE_FILE && mode & O_TRUNC) {
      itrunc(ip);
    }
    fp->type = TYPE_FILE;  // file_t don't and needn't distingush between file and dir
    fp->inode = ip;
    fp->offset = 0;
  } else if (type == TYPE_DEV) {
    fp->type = TYPE_DEV;
    fp->dev_op = dev_get(idevid(ip));
    iclose(ip);
    ip = NULL;
  } else if (type == TYPE_SYMLINK) {
    char buffer[MAX_NAME + 1] = {0};
    if (iread(ip, 0, buffer, MAX_NAME + 1) < 0)
      goto bad;

    iclose(ip);
    ip = NULL;

    // 递归打开符号链接指向的文件
    return fopen(buffer, mode, depth + 1);
  } else
    assert(0);
  fp->readable = !(mode & O_WRONLY);
  fp->writable = (mode & O_WRONLY) || (mode & O_RDWR);
  return fp;
bad:
  if (fp)
    fclose(fp);
  if (ip)
    iclose(ip);
  return NULL;
}

int fread(file_t* file, void* buf, uint32_t size) {
  // Lab3-1, distribute read operation by file's type
  // remember to add offset if type is FILE (check if iread return value >= 0!)
  if (!file->readable)
    return -1;

  // 检查 buf 的读权限
  PD* pgdir = vm_curr();
  size_t start = (size_t)buf;
  size_t end = start + size;

  for (size_t addr = start; addr < end; addr += PGSIZE) {
    // 检查读权限
    PTE* pte = vm_walkpte(pgdir, addr, 0);
    if (pte == NULL || pte->read_write == 0) {
      // 权限不足，调用 vm_pgfault 处理缺页错误
      vm_pgfault(addr, 2);
    }
  }

  int offset = -1;
  if (file->type == TYPE_FILE) {
    // 磁盘文件
    offset = iread(file->inode, file->offset, buf, size);
    if (offset == -1) {
      return -1;  // 读取失败
    }
    file->offset += offset;
  }
  if (file->type == TYPE_DEV) {
    // 设备文件
    offset = file->dev_op->read(buf, size);
  }
  if (file->type == TYPE_PIPE) {
    offset += pipe_read(file, buf, size);
  }
  return offset;
}

int fwrite(file_t* file, const void* buf, uint32_t size) {
  // Lab3-1, distribute write operation by file's type
  // remember to add offset if type is FILE (check if iwrite return value >= 0!)
  if (!file->writable)
    return -1;
  int offset = -1;
  if (file->type == TYPE_FILE) {  // 是磁盘文件
    offset = iwrite(file->inode, file->offset, buf, size);
    if (offset == -1) {
      return -1;
    }
    file->offset += offset;
  }
  if (file->type == TYPE_DEV) {  // 是设备文件
    offset = file->dev_op->write(buf, size);
  }
  if (file->type == TYPE_PIPE) {
    offset += pipe_write(file, buf, size);
  }
  return offset;
}

uint32_t fseek(file_t* file, uint32_t off, int whence) {
  // Lab3-1, change file's offset, do not let it cross file's size
  if (file->type == TYPE_FILE) {
    if (whence == SEEK_SET) {
      file->offset = off;
    }
    if (whence == SEEK_CUR) {
      file->offset += off;
    }
    if (whence == SEEK_END) {
      file->offset = isize(file->inode) + off;
    }
    return file->offset;
  }
  return -1;
}

file_t* fdup(file_t* file) {
  // Lab3-1, inc file's ref, then return itself
  file->ref++;
  return file;
}

void fclose(file_t* file) {
  // Lab3-1, dec file's ref, if ref==0 and it's a file, call iclose
  file->ref--;
  if (file->ref == 0 && file->type == TYPE_FILE) {
    iclose(file->inode);
  }
  if (file->type == TYPE_PIPE) {
    pipe_t* pipe = file->pipe;

    // 使用信号量保护对管道状态的修改
    sem_p(&pipe->mutex);

    if (file->readable) {
      pipe->read_open = 0;   // 关闭读口
      sem_v(&pipe->cv_buf);  // 唤醒可能在等待写缓冲的线程
    }
    if (file->writable) {
      pipe->write_open = 0;  // 关闭写口
      sem_v(&pipe->cv_buf);  // 唤醒可能在等待读缓冲的线程
    }

    // 如果读口和写口都关闭，释放管道资源
    if (pipe->read_open == 0 && pipe->write_open == 0) {
      sem_v(&pipe->mutex);  // 释放信号量
      pipe_close(pipe);     // 释放管道资源
      return;
    }

    sem_v(&pipe->mutex);  // 释放信号量
  }
}

int flink(const char* oldpath, const char* newpath) {
  inode_t* old_inode = iopen(oldpath, TYPE_NONE);
  if (!old_inode) {
    return -1;
  }

  if (ilink(newpath, old_inode) == NULL) {
    iclose(old_inode);
    return -1;
  }
  iclose(old_inode);
  return 0;
}

int fsymlink(const char* oldpath, const char* newpath) {
  // 检查 newpath 是否已经存在
  inode_t* new_inode = iopen(newpath, TYPE_NONE);
  if (new_inode) {
    iclose(new_inode);
    return -1;  // newpath 已经存在
  }

  // 创建 newpath 文件
  new_inode = iopen(newpath, TYPE_SYMLINK);
  if (!new_inode) {
    return -1;  // 创建失败
  }

  // 准备写入的路径内容
  char buffer[MAX_NAME + 1] = {0};  // 全部初始化为 0
  strncpy(buffer, oldpath, MAX_NAME);

  // 将路径写入符号链接文件
  if (iwrite(new_inode, 0, buffer, MAX_NAME + 1) < 0) {
    iclose(new_inode);
    return -1;  // 写入失败
  }

  iclose(new_inode);
  return 0;  // 成功
}

#define TOTAL_PIPE 32

static pipe_t pipes[TOTAL_PIPE];

static pipe_t* pipe_alloc() {
  // 找到一个空的管道并返回
  for (int i = 0; i < TOTAL_PIPE; i++) {
    if (pipes[i].read_open == 0 && pipes[i].write_open == 0) {
      return &pipes[i];
    }
  }
  return NULL;
}

void pipe_init(pipe_t* pipe) {
  // 初始化分配的管道
  pipe->read_pos = 0;
  pipe->write_pos = 0;

  pipe->read_open = 1;
  pipe->write_open = 1;

  pipe->empty = PIPE_SIZE;
  pipe->full = 0;

  sem_init(&pipe->mutex, 1);
  sem_init(&pipe->cv_buf, 0);

  memset(pipe->buffer, 0, PIPE_SIZE);  // 清空缓冲区
  pipe->no = 0;
}

int pipe_open(file_t* pipe_files[2]) {
  // TODO: WEEK11-link-pipe
  pipe_t* pipe = pipe_alloc();
  if (!pipe)
    return -1;

  // alloc read_side and write_side
  file_t* read_side = falloc();
  file_t* write_side = falloc();
  if (!read_side || !write_side) {
    if (read_side)
      fclose(read_side);
    if (write_side)
      fclose(write_side);
    return -1;
  }
  assert(read_side && write_side);

  // 设置文件结构
  read_side->type = TYPE_PIPE;
  read_side->pipe = pipe;
  read_side->inode = NULL;
  read_side->dev_op = NULL;
  read_side->readable = 1;
  read_side->writable = 0;
  read_side->offset = 0;
  read_side->ref = 1;

  write_side->type = TYPE_PIPE;
  write_side->pipe = pipe;
  write_side->inode = NULL;
  write_side->dev_op = NULL;
  write_side->readable = 0;
  write_side->writable = 1;
  write_side->offset = 0;
  write_side->ref = 1;

  // 初始化管道结构
  pipe_init(pipe);

  pipe_files[0] = read_side;
  pipe_files[1] = write_side;

  assert(pipe_files[0] && pipe_files[1]);

  return 0;
}

void pipe_close(pipe_t* pipe) {
  // TODO: WEEK11-link-pipe
  assert(pipe);
  pipe->read_open = 0;
  pipe->write_open = 0;
  memset(pipe->buffer, 0, PIPE_SIZE);
}

int pipe_write(file_t* file, const void* buf, uint32_t size) {
  pipe_t* pipe = file->pipe;
  const char* data = (const char*)buf;
  uint32_t written = 0;
  while (written < size) {
    sem_p(&pipe->mutex);
    // 写口关闭，返回错误
    if (!pipe->write_open) {
      sem_v(&pipe->mutex);
      return -1;
    }

    // 管道空位不足，等待
    while (pipe->empty == 0) {
      if (!pipe->read_open) {  // 读口关闭
        sem_v(&pipe->mutex);
        return written;
      }
      sem_v(&pipe->mutex);
      sem_p(&pipe->cv_buf);
      sem_p(&pipe->mutex);
    }
    // 写入数据到管道
    uint32_t writable = (size - written < pipe->empty) ? (size - written) : pipe->empty;
    for (uint32_t i = 0; i < writable; i++) {
      pipe->buffer[pipe->write_pos] = data[written++];
      pipe->write_pos = (pipe->write_pos + 1) % PIPE_SIZE;
    }
    pipe->full += writable;
    pipe->empty -= writable;

    sem_v(&pipe->cv_buf);  // 通知可能的读操作
    sem_v(&pipe->mutex);
  }

  return written;
}

int pipe_read(file_t* file, void* buf, uint32_t size) {
  pipe_t* pipe = file->pipe;
  char* data = (char*)buf;
  uint32_t read = 0;

  while (read < size) {
    sem_p(&pipe->mutex);
    // 读口关闭，返回错误
    if (!pipe->read_open) {
      sem_v(&pipe->mutex);
      return -1;
    }

    // 管道为空，等待
    while (pipe->full == 0) {
      if (!pipe->write_open) {  // 写口关闭
        sem_v(&pipe->mutex);
        return read;
      }
      sem_v(&pipe->mutex);
      sem_p(&pipe->cv_buf);
      sem_p(&pipe->mutex);
    }

    // 从管道读取数据
    uint32_t readable = (size - read < pipe->full) ? (size - read) : pipe->full;
    for (uint32_t i = 0; i < readable; i++) {
      data[read++] = pipe->buffer[pipe->read_pos];
      pipe->read_pos = (pipe->read_pos + 1) % PIPE_SIZE;
    }
    pipe->full -= readable;
    pipe->empty += readable;

    sem_v(&pipe->cv_buf);  // 通知可能的写操作
    sem_v(&pipe->mutex);
  }

  return read;
}
