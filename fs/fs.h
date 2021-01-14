#ifndef INCLUDE_FS_H_
#define INCLUDE_FS_H_

#include "type.h"

// ---------------
// | super block |
// ---------------
// |  inode map  |
// ---------------
// | sector map  |
// ---------------
// |    inode    |
// ---------------
// |    data     |
// ---------------

#define KFS_MAGIC (0x66666666)
#define OFFSET_256M (0X10000000)
#define OFFSET_512M (0x20000000)
#define OFFSET_1G (0x40000000)
#define OFFSET_2G (0x80000000)
#define OFFSET_3G (0xC0000000)

#define FS_SIZE (0x20000000)
// #define FS_SIZE (0x10000000)
#define OFFSET_FS OFFSET_512M
#define NUM_IMAP_SECTOR 1
#define SECTOR_SIZE 512
#define BLOCK_SIZE 0x1000   // 4KB
#define NUM_FD 15

#define ENTRY_SECTOR 8

#define MAX_DIRECT_NUM 10
#define MAX_NAME_LENGTH 20

#define NUM_BLOCK (FS_SIZE / BLOCK_SIZE)
#define NUM_INODE 32 * 1024
#define BLOCK_BITMAP_SIZE (NUM_BLOCK / 8)
#define INODE_BITMAP_SIZE BLOCK_SIZE

#define INIT_USED_BLOCK 1034    // 1 + 8 + 1 + 1K

#define TYPE_FILE   1
#define TYPE_DIR    2
#define TYPE_SELF   3
#define TYPE_FATHER 4
#define TYPE_ROOT   5

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR   3

#define BUFFER 0xffffffffa8000000

#define ROOT_ID 0

// offset is sector offset
typedef struct super_block
{
    uint32_t magic;
    uint32_t start_sector;
    uint32_t fs_size;
    uint32_t block_size;

    uint32_t block_bmp_offset;
    uint32_t inode_bmp_offset;
    uint32_t inode_offset;
    uint32_t data_offset;

    uint32_t total_inode_num;
    uint32_t free_inode_num;

    uint32_t total_block_num;
    uint32_t free_block_num;

    uint32_t inode_size;
    uint32_t dir_size;
} super_block_t;

typedef struct inode_entry 
{
    uint32_t id;    // inode number
    uint32_t type;
    uint32_t mode;
    uint32_t links_cnt;
    uint32_t fsize;
    uint32_t fnum;  // 目录中文件数
    uint32_t timestamp;
    uint32_t direct_table[MAX_DIRECT_NUM];
    uint32_t indirect_1_ptr;
    uint32_t indirect_2_ptr;
    uint32_t indirect_3_ptr;
    uint32_t padding[12];
} inode_entry_t;    // 32 * 4B = 128B

 
typedef struct dir_entry
{
    uint32_t id;
    uint32_t type;
    uint32_t mode;
    char name[MAX_NAME_LENGTH];
} dir_entry_t;      // 32B

typedef struct fd
{
    uint32_t inode_id;
    // uint32_t fsize;
    uint32_t mode;
    uint32_t r_offset;
    uint32_t w_offset;
} fd_t;

// extern inode_entry_t current_dir_entry;

void init_fs();

void do_mkfs();
int do_mkdir(char *name);
int do_rmdir(char *name);
int do_readdir(char *name);
int do_enterdir(char *name);
void do_statfs();
int do_mknod(char *name);
int do_open(char *name, uint32_t access);
int do_cat(char *name);
int do_write(uint32_t fd, char *buff, uint32_t size);
int do_read(uint32_t fd, char *buff, uint32_t size);
int do_close(uint32_t fd);

#endif