#include "fs.h"
#include "disk.h"
#include "klib.h"
#include "proc.h"

#ifdef EASY_FS

#define MAX_FILE (SECTSIZE / sizeof(dinode_t))
#define MAX_DEV 16
#define MAX_INODE (MAX_FILE + MAX_DEV)

// On disk inode
typedef struct dinode {
  uint32_t start_sect;
  uint32_t length;
  char name[MAX_NAME + 1];
} dinode_t;

// On OS inode, dinode with special info
struct inode {
  int valid;
  int type;
  int dev;  // dev_id if type==TYPE_DEV
  dinode_t dinode;
};

static inode_t inodes[MAX_INODE];

void init_fs() {
  dinode_t buf[MAX_FILE];
  read_disk(buf, 256);
  for (int i = 0; i < MAX_FILE; ++i) {
    inodes[i].valid = 1;
    inodes[i].type = TYPE_FILE;
    inodes[i].dinode = buf[i];
  }
}

inode_t* iopen(const char* path, int type) {
  for (int i = 0; i < MAX_INODE; ++i) {
    if (!inodes[i].valid)
      continue;
    if (strcmp(path, inodes[i].dinode.name) == 0) {
      return &inodes[i];
    }
  }
  return NULL;
}

int iread(inode_t* inode, uint32_t off, void* buf, uint32_t len) {
  assert(inode);
  char* cbuf = buf;
  char dbuf[SECTSIZE];
  uint32_t curr = -1;
  uint32_t total_len = inode->dinode.length;
  uint32_t st_sect = inode->dinode.start_sect;
  int i;
  for (i = 0; i < len && off < total_len; ++i, ++off) {
    if (curr != off / SECTSIZE) {
      read_disk(dbuf, st_sect + off / SECTSIZE);
      curr = off / SECTSIZE;
    }
    *cbuf++ = dbuf[off % SECTSIZE];
  }
  return i;
}

void iadddev(const char* name, int id) {
  assert(id < MAX_DEV);
  inode_t* inode = &inodes[MAX_FILE + id];
  inode->valid = 1;
  inode->type = TYPE_DEV;
  inode->dev = id;
  strcpy(inode->dinode.name, name);
}

uint32_t isize(inode_t* inode) {
  return inode->dinode.length;
}

int itype(inode_t* inode) {
  return inode->type;
}

uint32_t ino(inode_t* inode) {
  return inode - inodes;
}

int idevid(inode_t* inode) {
  return inode->type == TYPE_DEV ? inode->dev : -1;
}

int iwrite(inode_t* inode, uint32_t off, const void* buf, uint32_t len) {
  panic("write doesn't support");
}

void itrunc(inode_t* inode) {
  panic("trunc doesn't support");
}

inode_t* idup(inode_t* inode) {
  return inode;
}

void iclose(inode_t* inode) { /* do nothing */ }

int iremove(const char* path) {
  panic("remove doesn't support");
}

#else

#define DISK_SIZE (128 * 1024 * 1024)
#define BLK_NUM (DISK_SIZE / BLK_SIZE)

#define NDIRECT 12
#define NINDIRECT (BLK_SIZE / sizeof(uint32_t))

#define IPERBLK (BLK_SIZE / sizeof(dinode_t))  // inode num per blk

// super block
typedef struct super_block {
  uint32_t bitmap;  // block num of bitmap
  uint32_t istart;  // start block no of inode blocks
  uint32_t inum;    // total inode num
  uint32_t root;    // inode no of root dir
} sb_t;

// On disk inode
typedef struct dinode {
  uint16_t type;        // file type
  uint16_t link_count;  // link count for hard link
  union {
    uint32_t device;  // dev_id for device files
    uint32_t pipe;    // pipe addr for FIFO files
  };
  uint32_t size;                // file size
  uint32_t addrs[NDIRECT + 1];  // data block addresses, 12 direct and 1 first indirect
} dinode_t;

#define DINODE_NUM 128
typedef struct dinode_no {
  dinode_t dinode;
  uint32_t no;
} dinode_no_t;

static dinode_no_t open_dinodes[DINODE_NUM];

struct inode {
  int no;
  int ref;
  int del;
  dinode_t* dinode;  // 原来是dinode_t, 这里改成dinode_t *
};

#define SUPER_BLOCK 32
static sb_t sb;

void init_fs() {
  bread(&sb, sizeof(sb), SUPER_BLOCK, 0);
}

#define I2BLKNO(no) (sb.istart + no / IPERBLK)
#define I2BLKOFF(no) ((no % IPERBLK) * sizeof(dinode_t))

static void diread(dinode_t* di, uint32_t no) {
  bread(di, sizeof(dinode_t), I2BLKNO(no), I2BLKOFF(no));
}

static void diwrite(const dinode_t* di, uint32_t no) {
  bwrite(di, sizeof(dinode_t), I2BLKNO(no), I2BLKOFF(no));
}

static uint32_t dialloc(int type) {
  // Lab3-2: iterate all dinode, find a empty one (type==TYPE_NONE)
  // set type, clean other infos and return its no (remember to write back)
  // if no empty one, just abort
  // note that first (0th) inode always unused, because dirent's inode 0 mark invalid
  dinode_t dinode;
  for (uint32_t i = 1; i < sb.inum; ++i) {
    diread(&dinode, i);
    if (dinode.type == TYPE_NONE) {
      dinode.type = type;
      dinode.link_count = 1;
      diwrite(&dinode, i);
      return i;
    }
  }
  assert(0);
}

static void difree(uint32_t no) {
  dinode_t dinode;
  memset(&dinode, 0, sizeof dinode);
  diwrite(&dinode, no);
}

static uint32_t balloc() {
  // Lab3-2: iterate bitmap, find one free block
  // set the bit, clean the blk (can call bzero) and return its no
  // if no free block, just abort
  uint32_t byte;
  for (int i = 0; i < BLK_NUM / 32; ++i) {
    bread(&byte, 4, sb.bitmap, i * 4);
    if (byte != 0xffffffff) {
      for (int j = 0; j < 32; j++) {
        if ((byte & (1 << j)) == 0) {
          uint32_t free_id = 32 * i + j;
          bzero(free_id);
          byte = byte | (1 << j);
          bwrite(&byte, 4, sb.bitmap, i * 4);
          return free_id;
        }
      }
    }
  }
  assert(0);
}

static void bfree(uint32_t blkno) {
  // Lab3-2: clean the bit of blkno in bitmap
  assert(blkno >= 64);  // cannot free first 64 block
  uint32_t byte;
  uint32_t i = blkno / 32;
  uint32_t bit_count = blkno % 32;
  bread(&byte, 4, sb.bitmap, i * 4);
  byte &= ~(1 << bit_count);
  bwrite(&byte, 4, sb.bitmap, i * 4);
}

#define INODE_NUM 128
static inode_t inodes[INODE_NUM];

static inode_t* iget(uint32_t no) {
  // Lab3-2
  // if there exist one inode whose no is just no, inc its ref and return it
  // otherwise, find a empty inode slot, init it and return it
  // if no empty inode slot, just abort
  for (int i = 0; i < INODE_NUM; i++) {
    if (inodes[i].no == no) {
      inodes[i].ref++;
      return &inodes[i];
    }
  }
  for (int i = 0; i < INODE_NUM; i++) {
    if (inodes[i].ref == 0) {
      inodes[i].no = no;
      inodes[i].ref = 1;
      inodes[i].del = 0;
      for (int j = 1; j < DINODE_NUM; j++) {
        if (open_dinodes[j].no == 0 || open_dinodes[j].no == no) {
          open_dinodes[j].no = no;
          inodes[i].dinode = &open_dinodes[j].dinode;
          diread(inodes[i].dinode, no);
          return &inodes[i];
        }
      }
    }
  }
  assert(0);
  return NULL;
}

static void iupdate(inode_t* inode) {
  // Lab3-2: sync the inode->dinode to disk
  // call me EVERYTIME after you edit inode->dinode
  diwrite(inode->dinode, inode->no);
}

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return NULL.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = NULL
//
static const char* skipelem(const char* path, char* name) {
  const char* s;
  int len;
  while (*path == '/')
    path++;
  if (*path == 0)
    return 0;
  s = path;
  while (*path != '/' && *path != 0)
    path++;
  len = path - s;
  if (len >= MAX_NAME) {
    memcpy(name, s, MAX_NAME);
    name[MAX_NAME] = 0;
  } else {
    memcpy(name, s, len);
    name[len] = 0;
  }
  while (*path == '/')
    path++;
  return path;
}

static void idirinit(inode_t* inode, inode_t* parent) {
  // Lab3-2: init the dir inode, i.e. create . and .. dirent
  assert(inode->dinode->type == TYPE_DIR);
  assert(parent->dinode->type == TYPE_DIR);  // both should be dir
  assert(inode->dinode->size == 0);          // inode shoule be empty
  dirent_t dirent;
  // set .
  dirent.inode = inode->no;
  strcpy(dirent.name, ".");
  iwrite(inode, 0, &dirent, sizeof dirent);
  // set ..
  dirent.inode = parent->no;
  strcpy(dirent.name, "..");
  iwrite(inode, sizeof dirent, &dirent, sizeof dirent);
}

static inode_t* ilookup(inode_t* parent, const char* name, uint32_t* off, int type) {
  // Lab3-2: iterate the parent dir, find a file whose name is name
  // if off is not NULL, store the offset of the dirent_t to it
  // if no such file and type == TYPE_NONE, return NULL
  // if no such file and type != TYPE_NONE, create the file with the type
  assert(parent->dinode->type == TYPE_DIR);  // parent must be a dir
  dirent_t dirent;
  uint32_t size = parent->dinode->size, empty = size;
  for (uint32_t i = 0; i < size; i += sizeof dirent) {
    // directory is a file containing a sequence of dirent structures
    iread(parent, i, &dirent, sizeof dirent);
    if (dirent.inode == 0) {
      // a invalid dirent, record the offset (used in create file), then skip
      if (empty == size)
        empty = i;
      continue;
    }
    // a valid dirent, compare the name
    if (strcmp(dirent.name, name) == 0) {
      if (off != NULL) {
        *off = i;
      }
      return iget(dirent.inode);
    }
  }
  // not found
  if (type == TYPE_NONE)
    return NULL;
  // need to create the file, first alloc inode, then init dirent, write it to parent
  // if you create a dir, remember to init it's . and ..
  inode_t* inode = iget(dialloc(type));
  if (type == TYPE_DIR) {
    idirinit(inode, parent);
  }
  dirent_t new_dirent;
  new_dirent.inode = inode->no;
  strcpy(new_dirent.name, name);
  iwrite(parent, empty, &new_dirent, sizeof(new_dirent));
  if (off != NULL) {
    *off = empty;
  }
  return inode;
}

static inode_t* iopen_parent(const char* path, char* name) {
  // Lab3-2: open the parent dir of path, store the basename to name
  // if no such parent, return NULL
  inode_t *ip, *next;
  // set search starting inode
  if (path[0] == '/') {
    ip = iget(sb.root);
  } else {
    ip = idup(proc_curr()->group_leader->cwd);
  }
  assert(ip);
  while ((path = skipelem(path, name))) {
    // curr round: need to search name in ip
    if (ip->dinode->type != TYPE_DIR) {
      // not dir, cannot search
      iclose(ip);
      return NULL;
    }
    if (*path == 0) {
      // last search, return ip because we just need parent
      return ip;
    }
    // not last search, need to continue to find parent
    next = ilookup(ip, name, NULL, 0);
    if (next == NULL) {
      // name not exist
      iclose(ip);
      return NULL;
    }
    iclose(ip);
    ip = next;
  }
  iclose(ip);
  return NULL;
}

inode_t* iopen(const char* path, int type) {
  // Lab3-2: if file exist, open and return it
  // if file not exist and type==TYPE_NONE, return NULL
  // if file not exist and type!=TYPE_NONE, create the file as type
  char name[MAX_NAME + 1];
  if (skipelem(path, name) == NULL) {
    // no parent dir for path, path is "" or "/"
    // "" is an invalid path, "/" is root dir
    return path[0] == '/' ? iget(sb.root) : NULL;
  }
  // path do have parent, use iopen_parent and ilookup to open it
  // remember to close the parent inode after you ilookup it
  inode_t* parent = iopen_parent(path, name);
  if (parent == NULL) {
    return NULL;
  }
  inode_t* current_inode = ilookup(parent, name, NULL, type);
  iclose(parent);
  return current_inode;
}

static uint32_t iwalk(inode_t* inode, uint32_t no) {
  // return the blkno of the file's data's no th block, if no, alloc it
  if (no < NDIRECT) {
    // direct address
    if (inode->dinode->addrs[no]) {
      return inode->dinode->addrs[no];
    }
    uint32_t new_no = balloc();
    inode->dinode->addrs[no] = new_no;
    return new_no;
  }
  no -= NDIRECT;
  if (no < NINDIRECT) {
    // indirect address
    if (inode->dinode->addrs[NDIRECT] == 0) {
      inode->dinode->addrs[NDIRECT] = balloc();
      iupdate(inode);
    }
    uint32_t new_no;
    bread(&new_no, 4, inode->dinode->addrs[NDIRECT], 4 * no);
    if (new_no == 0) {
      new_no = balloc();
      bwrite(&new_no, 4, inode->dinode->addrs[NDIRECT], 4 * no);
      iupdate(inode);
    }
    return new_no;
  }
  assert(0);  // file too big, not need to handle this case
}

int iread(inode_t* inode, uint32_t off, void* buf, uint32_t len) {
  // Lab3-2: read the inode's data [off, MIN(off+len, size)) to buf
  // use iwalk to get the blkno and read blk by blk
  uint32_t total_read = 0;
  uint32_t end_off = off + len;
  uint32_t file_size = inode->dinode->size;

  if (end_off > file_size) {
    end_off = file_size;
    len = end_off - off;
  }

  while (len > 0 && off < end_off) {
    uint32_t blkno;
    uint32_t blk_offset = off % BLK_SIZE;
    uint32_t bytes_left = end_off - off;
    uint32_t blk_space_left = BLK_SIZE - blk_offset;
    uint32_t bytes_to_read = bytes_left < blk_space_left ? bytes_left : blk_space_left;

    blkno = iwalk(inode, off / BLK_SIZE);
    bread(buf, bytes_to_read, blkno, blk_offset);
    len -= bytes_to_read;
    off += bytes_to_read;
    buf += bytes_to_read;
    total_read += bytes_to_read;
  }
  return total_read;
}

int iwrite(inode_t* inode, uint32_t off, const void* buf, uint32_t len) {
  // Lab3-2: write buf to the inode's data [off, off+len)
  // if off>size, return -1 (can not cross size before write)
  // if off+len>size, update it as new size (but can cross size after write)
  // use iwalk to get the blkno and read blk by blk
  uint32_t total_witten = 0;
  uint32_t end_off = off + len;
  uint32_t file_size = inode->dinode->size;

  if (off > file_size) {
    return -1;
  }
  if (end_off > file_size) {
    inode->dinode->size = end_off;
    iupdate(inode);
  }
  while (total_witten < len) {
    uint32_t blkno;
    uint32_t blk_offset = off % BLK_SIZE;
    uint32_t bytes_left = len - total_witten;
    uint32_t blk_space_left = BLK_SIZE - blk_offset;

    uint32_t bytes_to_write = (bytes_left < blk_space_left) ? bytes_left : blk_space_left;

    blkno = iwalk(inode, off / BLK_SIZE);

    bwrite(buf, bytes_to_write, blkno, blk_offset);

    off += bytes_to_write;
    buf += bytes_to_write;
    total_witten += bytes_to_write;
  }
  iupdate(inode);
  return total_witten;
}

void itrunc(inode_t* inode) {
  // Lab3-2: free all data block used by inode (direct and indirect)
  // mark all address of inode 0 and mark its size 0
  size_t total_num = inode->dinode->size / BLK_SIZE + 1;
  for (int blk_no = 0; blk_no < total_num; blk_no++) {
    if (inode->dinode->addrs[blk_no] != 0) {
      bfree(inode->dinode->addrs[blk_no]);
    }
    if (blk_no < NDIRECT) {
      inode->dinode->addrs[blk_no] = 0;
    } else {
      uint32_t no = 0;
      blk_no -= NDIRECT;
      bread(&no, 4, inode->dinode->addrs[NDIRECT], 4 * blk_no);
      if (no != 0) {
        bfree(no);
      }
    }
    inode->dinode->size = 0;
    iupdate(inode);
  }
}

inode_t* idup(inode_t* inode) {
  assert(inode);
  inode->ref += 1;
  return inode;
}

void iclose(inode_t* inode) {
  assert(inode);
  if (inode->ref == 1 && inode->del) {
    inode->dinode->link_count--;
    // 如果dinode的link_count为0，释放dinode
    if (!inode->dinode->link_count) {
      difree(inode->no);
      if (inode->dinode->type == TYPE_FIFO) {
        rmfifo(inode->no);
      }
      itrunc(inode);
      for (int i = 0; i < DINODE_NUM; i++) {  // 在活动dinode数组中找到对应的项并释放它
        if (&(open_dinodes[i].dinode) == inode->dinode) {
          open_dinodes[i].no = 0;
          inode->dinode = NULL;
        }
      }
    }
  }
  inode->ref -= 1;
}

uint32_t isize(inode_t* inode) {
  return inode->dinode->size;
}

int itype(inode_t* inode) {
  return inode->dinode->type;
}

uint32_t ino(inode_t* inode) {
  return inode->no;
}

int idevid(inode_t* inode) {
  return itype(inode) == TYPE_DEV ? inode->dinode->device : -1;
}

void iadddev(const char* name, int id) {
  inode_t* ip = iopen(name, TYPE_DEV);
  assert(ip);
  ip->dinode->device = id;
  iupdate(ip);
  iclose(ip);
}

static int idirempty(inode_t* inode) {
  // Lab3-2: return whether the dir of inode is empty
  // the first two dirent of dir must be . and ..
  // you just need to check whether other dirent are all invalid
  assert(inode->dinode->type == TYPE_DIR);  // 确保 inode 是目录类型

  dirent_t dirent;
  uint32_t size = inode->dinode->size;

  // 从第三个目录项开始检查
  for (uint32_t i = sizeof(dirent) * 2; i < size; i += sizeof(dirent)) {
    iread(inode, i, &dirent, sizeof(dirent));  // 读取目录项
    if (dirent.inode != 0) {                   // 如果发现有效的目录项
      return 0;                                // 目录非空
    }
  }

  return 1;  // 目录为空
}

int iremove(const char* path) {
  // Lab3-2: remove the file, return 0 on success, otherwise -1
  // first open its parent, if no parent, return -1
  // then find file in parent, if not exist, return -1
  // if the file need to remove is a dir, only remove it when it's empty
  // . and .. cannot be remove, so check name set by iopen_parent
  // remove a file just need to clean the dirent points to it and set its inode's del
  // the real remove will be done at iclose, after everyone close it

  char name[MAX_NAME + 1];  // 用于存储文件名
  inode_t* parent_inode;
  uint32_t off;

  // 获取文件的父目录和文件名
  parent_inode = iopen_parent(path, name);  // 使用iopen_parent获取父目录并存储文件名
  if (parent_inode == NULL) {
    return -1;  // 没有找到父目录
  }

  // 不允许删除 . 和 .. 文件
  if (!(strcmp(name, ".") && strcmp(name, ".."))) {
    iclose(parent_inode);  // 关闭父目录
    return -1;             // 不允许删除 . 或 .. 文件
  }

  // 查找文件并确保其存在
  inode_t* inode = ilookup(parent_inode, name, &off, TYPE_NONE);
  if (inode == NULL) {
    iclose(parent_inode);  // 关闭父目录
    return -1;             // 文件不存在
  }

  // 如果文件是目录，检查目录是否为空
  if (inode->dinode->type == TYPE_DIR) {
    if (idirempty(inode) == 0) {
      iclose(parent_inode);  // 关闭父目录
      iclose(inode);         // 关闭目录
      return -1;             // 目录非空，不能删除
    }
  }

  // diwrite(&dinode, inode->no);
  if (inode->dinode->link_count > 0) {
    inode->dinode->link_count--;
  }
  if (inode->dinode->link_count == 0) {
    inode->del = 1;  // 将 inode 的 del 标记设置为 1，表示删除

    // 清空目录项
    dirent_t dirent;
    memset(&dirent, 0, sizeof(dirent));                  // 清空目录项内容
    iwrite(parent_inode, off, &dirent, sizeof(dirent));  // 写回父目录
  }

  return 0;
}

inode_t* ilink(const char* path, inode_t* old_inode) {
  char name[MAX_NAME + 1];

  inode_t* parent_dir = iopen_parent(path, name);
  if (parent_dir == NULL) {
    return NULL;
  }
  assert(parent_dir->dinode->type == TYPE_DIR);

  // find empty dirent
  dirent_t dirent;
  uint32_t size = parent_dir->dinode->size, empty = size;
  for (uint32_t i = 0; i < size; i += sizeof dirent) {
    if (dirent.inode == 0) {
      if (empty == size)
        empty = i;
      continue;
    }
    if (strcmp(dirent.name, name) == 0) {
      iclose(parent_dir);
      return NULL;
    }
  }
  assert(empty == size);

  // find empty inode
  inode_t* inode = NULL;
  for (int i = 0; i < INODE_NUM; i++) {
    if (inodes[i].ref == 0) {
      inode = &inodes[i];
      break;
    }
  }
  assert(inode);

  // initialize inode
  inode->no = old_inode->no;
  inode->ref = 1;
  inode->del = 0;
  inode->dinode = old_inode->dinode;
  ++inode->dinode->link_count;
  iupdate(inode);

  // write dirent
  dirent.inode = inode->no;
  strcpy(dirent.name, name);
  iwrite(parent_dir, empty, &dirent, sizeof dirent);

  return inode;
}

int ififoaddr(inode_t* inode) {
  return inode->dinode->pipe;
}

void isetfifo(inode_t* ip, void* pipe) {
  ip->dinode->pipe = (uint32_t)pipe;
  iupdate(ip);
}

#endif
