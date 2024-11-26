#include "file.h"
#include "klib.h"
#include "proc.h"
#include "vme.h"

#define TOTAL_FILE 128

file_t files[TOTAL_FILE];
static pipe_t* pipe_alloc();
static void pipe_init(pipe_t* pipe);

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
  if (++depth > 40) {
    return NULL;
  }
  file_t* fp = falloc();
  inode_t* ip = NULL;
  if (!fp)
    goto bad;
  // Lab3-2, determine type according to mode
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
    // Lab3-2, if type is not DIR, go bad if mode&O_DIR
    if (type != TYPE_DIR && (mode & O_DIR)) {
      goto bad;
    }
    // Lab3-2, if type is DIR, go bad if mode WRITE or TRUNC
    if (type == TYPE_DIR && (mode & O_WRONLY || mode & O_RDWR || mode & O_TRUNC)) {
      goto bad;
    }
    // Lab3-2, if mode&O_TRUNC, trunc the file
    if (mode & O_TRUNC) {
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
    char path[MAX_NAME + 1];
    memset(path, 0, sizeof path);
    iread(ip, 0, path, MAX_NAME + 1);
    iclose(ip);
    return fopen(path, mode, depth + 1);
  } else if (type == TYPE_FIFO) {
    fp->type = TYPE_FIFO;
    fp->pipe = (pipe_t*)ififoaddr(ip);
    fp->inode = ip;
    if (fp->pipe->no != ino(fp->inode)) {
      pipe_t* pipe = pipe_alloc();
      if (pipe == NULL) {
        goto bad;
      }
      pipe_init(pipe);
      fp->pipe = pipe;
      isetfifo(fp->inode, pipe);
      pipe->no = ino(fp->inode);
    }
  } else {
    assert(0);
  }
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

  int read_bytes = -1;
  if (file->type == TYPE_FILE) {
    // 磁盘文件
    read_bytes = iread(file->inode, file->offset, buf, size);
    if (read_bytes == -1) {
      return -1;  // 读取失败
    }
    file->offset += read_bytes;
  }
  if (file->type == TYPE_DEV) {
    // 设备文件
    read_bytes = file->dev_op->read(buf, size);
  }
  if (file->type == TYPE_PIPE) {
    read_bytes = pipe_read(file, buf, size);
  }
  if (file->type == TYPE_FIFO) {
    read_bytes = pipe_read(file, buf, size);
  }
  return read_bytes;
}

int fwrite(file_t* file, const void* buf, uint32_t size) {
  // Lab3-1, distribute write operation by file's type
  // remember to add offset if type is FILE (check if iwrite return value >= 0!)
  if (!file->writable)
    return -1;
  int write_size = -1;
  if (file->type == TYPE_FILE) {  // 是磁盘文件
    write_size = iwrite(file->inode, file->offset, buf, size);
    if (write_size == -1) {
      return -1;
    }
    file->offset += write_size;
  }
  if (file->type == TYPE_DEV) {  // 是设备文件
    write_size = file->dev_op->write(buf, size);
  }
  if (file->type == TYPE_PIPE) {
    write_size = pipe_write(file, buf, size);
  }
  if (file->type == TYPE_FIFO) {
    write_size = pipe_write(file, buf, size);
  }
  return write_size;
}

uint32_t fseek(file_t* file, uint32_t off, int whence) {
  // Lab3-1, change file's offset, do not let it cross file's size
  if (file->type == TYPE_FILE) {
    if (whence == SEEK_SET) {
      file->offset = off;
    } else if (whence == SEEK_CUR) {
      file->offset += off;
    } else if (whence == SEEK_END) {
      file->offset = isize(file->inode) + off;
    } else {
      return -1;
    }
    return file->offset;
  }
  return -1;
}

file_t* fdup(file_t* file) {
  // Lab3-1, inc file's ref, then return itself
  if (file) {
    file->ref++;
  }
  return file;
}

void fclose(file_t* file) {
  file->ref--;
  if (file && file->ref == 0) {
    if (file->type == TYPE_FILE || file->type == TYPE_FIFO) {
      iclose(file->inode);
    } else if (file->type == TYPE_PIPE) {
      pipe_t* pipe = file->pipe;
      sem_p(&pipe->mutex);
      if (file->readable) {
        pipe->read_open = 0;
      }
      if (file->writable) {
        pipe->write_open = 0;
      }
      if (pipe->read_open == 0 && pipe->write_open == 0) {
        pipe_close(pipe);
      }
      while (pipe->cv_buf.value < 0) {
        sem_v(&pipe->cv_buf);
      }
      sem_v(&pipe->mutex);
    }
    file->type = TYPE_NONE;
  }
}

int flink(const char* oldpath, const char* newpath) {
  inode_t* old_inode = iopen(oldpath, TYPE_NONE);
  if (old_inode == NULL) {
    return -1;
  }
  inode_t* new_inode = ilink(newpath, old_inode);
  iclose(old_inode);
  if (new_inode == NULL) {
    return -1;
  }
  iclose(new_inode);
  return 0;
}

int fsymlink(const char* oldpath, const char* newpath) {
  if (fopen(newpath, O_RDONLY, 0)) {
    return -1;
  }

  inode_t* inode = iopen(newpath, TYPE_SYMLINK);
  if (inode == NULL) {
    return -1;
  }

  char path[MAX_NAME + 1];
  memset(path, 0, sizeof path);
  size_t len = strlen(oldpath);
  memcpy(path, oldpath, len);
  path[len] = '\0';
  iwrite(inode, 0, path, len + 1);
  iclose(inode);
  return 0;
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
  sem_t* cv_buf = &pipe->cv_buf;
  uint32_t write_bytes = 0;

  const char* data = (const char*)buf;
  sem_p(&pipe->mutex);
  while (write_bytes < size) {
    if (pipe->write_open == 0) {
      sem_v(&pipe->mutex);
      return -1;
    }
    if (pipe->read_open == 0) {
      sem_v(&pipe->mutex);
      return write_bytes;
    }

    if (pipe->empty == 0) {
      sem_v(&pipe->mutex);
      if (cv_buf->value < 0) {
        sem_v(cv_buf);
      }
      sem_p(cv_buf);
      sem_p(&pipe->mutex);
      continue;
    }

    pipe->buffer[pipe->write_pos] = data[write_bytes];
    pipe->write_pos = (pipe->write_pos + 1) % PIPE_SIZE;
    write_bytes++;
    pipe->full++;
    pipe->empty--;
  }
  if (cv_buf->value < 0) {
    sem_v(cv_buf);
  }
  sem_v(&pipe->mutex);

  return write_bytes;
}

int pipe_read(file_t* file, void* buf, uint32_t size) {
  pipe_t* pipe = file->pipe;
  sem_t* mutex = &pipe->mutex;
  sem_t* cv_buf = &pipe->cv_buf;
  uint32_t read_bytes = 0;

  sem_p(mutex);
  while (pipe->full == 0) {
    if (pipe->read_open == 0) {
      sem_v(mutex);
      return -1;
    }
    if (pipe->write_open == 0 && pipe->full == 0) {
      sem_v(mutex);
      return read_bytes;
    }
    sem_v(mutex);
    if (cv_buf->value < 0) {
      sem_v(cv_buf);
    }
    sem_p(cv_buf);
    sem_p(mutex);
    continue;
  }

  size = MIN(size, pipe->full);
  while (read_bytes < size) {
    ((char*)buf)[read_bytes] = pipe->buffer[pipe->read_pos];
    pipe->read_pos = (pipe->read_pos + 1) % PIPE_SIZE;
    read_bytes++;
    pipe->full--;
    pipe->empty++;
    sem_v(mutex);
  }
  if (cv_buf->value < 0) {
    sem_v(cv_buf);
  }
  sem_v(mutex);
  return read_bytes;
}

file_t* mkfifo(const char* path, int mode) {
  if (fopen(path, O_RDONLY, 0)) {
    return NULL;  // 检查是否已经存在
  }
  inode_t* inode = iopen(path, TYPE_FIFO);
  if (!inode)
    return NULL;

  pipe_t* pipe = pipe_alloc();
  if (pipe == NULL) {
    iclose(inode);
    return NULL;
  }

  pipe_init(pipe);
  isetfifo(inode, pipe);

  file_t* fp = falloc();
  if (fp == NULL) {
    iclose(inode);
    return NULL;
  }
  fp->type = TYPE_PIPE;
  fp->ref = 1;
  fp->pipe = pipe;
  fp->inode = inode;
  fp->dev_op = NULL;
  fp->readable = !(mode & O_WRONLY);
  fp->writable = (mode & O_WRONLY) || (mode & O_RDWR);
  fp->offset = 0;

  pipe->no = ino(inode);
  iclose(inode);
  return fp;
}

void rmfifo(int no) {
  for (int i = 0; i < TOTAL_PIPE; i++) {
    if (pipes[i].no == no) {
      pipe_close(&pipes[i]);
    }
  }
}
