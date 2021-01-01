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
#define MAX_NAME_LENGTH 32

#define TYPE_FILE 1
#define TYPE_DIR  2
#define TYPE_.    3
#define TYPE_..   4

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR   3

// offset is sector offset
typedef struct super_block
{
    uint32_t magic;
    uint32_t fs_size;
    uint32_t block_size;

    uint32_t block_bmp_offset;
    uint32_t block_bmp_num;

    uint32_t inode_bmp_offset;
    uint32_t inode_bmp_num;

    uint32_t inode_offset;
    uint32_t inode_num;

    uint32_t data_block_offset;
    uint32_t data_block_num;

    uint32_t inode_free_num;
    uint32_t data_block_free_num;

    uint32_t inode_size;
    uint32_t dentry_size;
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
    uint32_t padding[28];
} dir_entry_t;      // 32 * 4B = 128B

typedef struct fd
{
    uint32_t inode_id;
    uint32_t fsize;
    uint32_t mode;
    uint32_t r_offset;
    uint32_t w_offset;
    // uint32_t direct_table[MAX_DIRECT_NUM];
    // uint32_t indirect_1_ptr;
    // uint32_t indirect_2_ptr;
    // uint32_t indirect_3_ptr;
} fd_t;

// extern inode_entry_t current_dir_entry;

void init_fs();
 
void read_super_block();

 
int readdir(char *name);
int mkdir(char *name);
int rmdir(char *name);
int mknod(char *name);

int open(char *name, uint32_t access);
int write(uint32_t fd, char *buff, uint32_t size);
int read(uint32_t fd, char *buff, uint32_t size);
int close(uint32_t fd);
int cat(char *name);

#endif