#include "fs.h"
#include "string.h"
#include "common.h"

/*
* File System
* FS size    : 1GB
* Block size : 4KB
* Inode size : 128B
* Inode Bitmap size : 32K / 8 * 1B= 4KB
* Block Bitmap size : 256K / 8 * 1B= 32KB
* -----------------------------------------------------------------------------
* | Superblock   | Block Bitmap  | Inode Bitmap  |      Inode      |  Blocks  |
* | 1 Block 4KB  | 8 Blocks 32KB | 1 Block  4KB  | 1024Blocks 4MB  |  Others  |
* -----------------------------------------------------------------------------
*/

// static uint8_t zero[SECTOR_SIZE]; // always zero
static uint8_t superblock_buffer[SECTOR_SIZE];
static uint8_t blockbmp_buffer[BLOCK_BITMAP_SIZE];
static uint8_t inodebmp_buffer[INODE_BITMAP_SIZE];
// static uint8_t inode_buffer[BLOCK_SIZE];
static inode_entry_t inode_buffer;
static super_block_t *superblock = (super_block_t *)superblock_buffer;

static fd_t fds[NUM_FD];
static inode_entry_t current_dir_entry;

void read_superblock()
{
    sd_card_read(superblock_buffer, OFFSET_FS, SECTOR_SIZE);
}

void write_superblock()
{
    sd_card_write(superblock_buffer, OFFSET_FS, SECTOR_SIZE);
}

void read_blockbmp()
{
    sd_card_read(blockbmp_buffer, superblock->block_bmp_offset, BLOCK_BITMAP_SIZE);
}

void write_blockbmp()
{
    sd_card_write(blockbmp_buffer, superblock->block_bmp_offset, BLOCK_BITMAP_SIZE);
}

void read_inodebmp()
{
    sd_card_read(inodebmp_buffer, superblock->inode_bmp_offset, INODE_BITMAP_SIZE);
}

void write_inodebmp()
{
    sd_card_write(inodebmp_buffer, superblock->inode_bmp_offset, INODE_BITMAP_SIZE);
}

void read_inode(uint32_t id)
{
    uint32_t sector = id / (SECTOR_SIZE / sizeof(inode_entry_t));
    uint32_t part = id % (SECTOR_SIZE / sizeof(inode_entry_t));
    sd_card_read((void *)BUFFER, superblock->inode_offset + sector * SECTOR_SIZE, SECTOR_SIZE);
    memcpy((uint8_t *)&inode_buffer, (uint8_t *)(BUFFER + part * sizeof(inode_entry_t)), sizeof(inode_entry_t));
}

void write_inode(uint32_t id)
{
    uint32_t sector = id / (SECTOR_SIZE / sizeof(inode_entry_t));
    uint32_t part = id % (SECTOR_SIZE / sizeof(inode_entry_t));
    sd_card_read((void *)BUFFER, superblock->inode_offset + sector * SECTOR_SIZE, SECTOR_SIZE);
    memcpy((uint8_t *)(BUFFER + part * sizeof(inode_entry_t)), (uint8_t *)&inode_buffer, sizeof(inode_entry_t));
    sd_card_write((void *)BUFFER, superblock->inode_offset + sector * SECTOR_SIZE, SECTOR_SIZE);
}

void read_block(uint32_t id)
{
    sd_card_read((void *)BUFFER, OFFSET_FS + id * BLOCK_SIZE, BLOCK_SIZE);
}

void write_block(uint32_t id)
{
    sd_card_write((void *)BUFFER, OFFSET_FS + id * BLOCK_SIZE, BLOCK_SIZE);
}

uint32_t alloc_inode()
{
    read_inodebmp();
    int i, j;
    for (i = 0; i < INODE_BITMAP_SIZE; i++) {
        if (inodebmp_buffer[i] != 0xff) {
            break;
        }
    }
    uint8_t temp = inodebmp_buffer[i];
    for (j = 0; j < 8; j++) {
        if ((temp & 0x01) == 0) {
            break;
        }
        else {
            temp = temp >> 1;
        }
    }
    temp = 0x01 << j;
    inodebmp_buffer[i] |= temp;
    write_inodebmp();
    return (8 * i + j);
}

void free_inode(uint32_t id)
{
    int i = id / 8;
    int j = id % 8;
    uint8_t temp = 0x01 << j;
    read_inodebmp();
    inodebmp_buffer[i] &= (~temp);
    write_inodebmp();
}

uint32_t alloc_block()
{
    read_blockbmp();
    int i, j;
    for (i = 0; i < BLOCK_BITMAP_SIZE; i++) {
        if (blockbmp_buffer[i] != 0xff) {
            break;
        }
    }
    uint8_t temp = blockbmp_buffer[i];
    for (j = 0; j < 8; j++) {
        if ((temp & 0x01) == 0) {
            break;
        }
        else {
            temp = temp >> 1;
        }
    }
    temp = 0x01 << j;
    blockbmp_buffer[i] |= temp;
    write_blockbmp();
    return (8 * i + j);
}

void free_block(uint32_t id)
{
    int i = id / 8;
    int j = id % 8;
    uint8_t temp = 0x01 << j;
    read_blockbmp();
    blockbmp_buffer[i] &= (~temp);
    write_blockbmp();
}

void init_superblock()
{
    bzero(superblock_buffer, SECTOR_SIZE);
    superblock->magic = KFS_MAGIC;
    superblock->start_sector = OFFSET_FS;
    superblock->fs_size = FS_SIZE;
    superblock->block_size = BLOCK_SIZE;
    superblock->block_bmp_offset = OFFSET_FS + BLOCK_SIZE;
    superblock->inode_bmp_offset = superblock->block_bmp_offset + BLOCK_BITMAP_SIZE;
    superblock->inode_offset = superblock->inode_bmp_offset + INODE_BITMAP_SIZE;
    superblock->data_offset = superblock->inode_offset + NUM_INODE * sizeof(inode_entry_t);
    superblock->total_inode_num = NUM_INODE;
    superblock->free_inode_num = NUM_INODE;
    superblock->total_block_num = NUM_BLOCK - INIT_USED_BLOCK;
    superblock->free_block_num = NUM_BLOCK;
    superblock->inode_size = sizeof(inode_entry_t);
    superblock->dir_size = sizeof(dir_entry_t);
    write_superblock();
}

void init_blockbmp()
{
    bzero(blockbmp_buffer, BLOCK_BITMAP_SIZE);
    int i, j;
    for (i = 0; i < INIT_USED_BLOCK / 8; i++) {
        blockbmp_buffer[i] = 0xff;
    }
    for (j = 0; j < INIT_USED_BLOCK % 8; i++) {
        blockbmp_buffer[i] |= (0x01 << j);
    }
    write_blockbmp();
}

void init_inodebmp()
{
    bzero(inodebmp_buffer, INODE_BITMAP_SIZE);
    write_inodebmp();
}

void init_inode(uint32_t block, uint32_t father, uint32_t self, uint32_t mode, uint32_t type)
{
    inode_buffer.id = self;
    inode_buffer.type = type;
    inode_buffer.mode = mode;
    inode_buffer.links_cnt = 0;
    if (type == TYPE_DIR || type == TYPE_ROOT) {
        inode_buffer.fsize = 2 * sizeof(dir_entry_t);
    }
    else {
        inode_buffer.fsize = 0;
    }
    inode_buffer.fnum = 0;
    inode_buffer.direct_table[0] = block;
    int i;
    for (i = 1; i < MAX_DIRECT_NUM; i++) {
        inode_buffer.direct_table[i] = 0;
    }
    inode_buffer.indirect_1_ptr = 0;
    inode_buffer.indirect_2_ptr = 0;
    inode_buffer.indirect_3_ptr = 0;

    write_inode(inode_buffer.id);

    if (inode_buffer.type != TYPE_ROOT) {
        read_inode(father);
        uint32_t fnum = ++(inode_buffer.fnum);
        write_inode(father);
        
        uint32_t block = inode_buffer.direct_table[0];
        read_block(block);
        dir_entry_t* dir;
        int i;
        for (i = 0; i < fnum; i++) {
            dir = (dir_entry_t*)(BUFFER + i * sizeof(dir_entry_t));
            if (dir->name[0] == '\0') break;
        }
        // while (inst_path[0][j] != '\0') {
        //     dir->name[j] = inst_path[0][j];
        //     j++;
        // }
        dir->name[i] = '\0';
        dir->id = self;
        dir->type = type;

        write_block(block);
    }
}

// init directory block
void init_dir_block(uint32_t block, uint16_t father, uint16_t self)
{
    read_block(block);
    
    dir_entry_t * dir = (dir_entry_t *)BUFFER;
    dir->name[0] = '.';
    dir->name[1] = '.';
    dir->name[2] = '\0';
    dir->id = father;
    dir->type = TYPE_FATHER;
    
    dir = (dir_entry_t *)(BUFFER + sizeof(dir_entry_t));
    dir->name[0] = '.';
    dir->name[1] = '\0';
    dir->id = self;
    dir->type = TYPE_SELF;
    
    write_block(block);
}

void init_rootdir()
{
    uint32_t self = alloc_inode();
    if (self != 0) {
        kprintf("[ERROR] ROOT DIR INODE NUMBER = %d, NOT ZERO\n", self);
    }
    uint32_t block = alloc_block();
    init_inode(block, self, self, O_RDWR, TYPE_ROOT);
    init_dir_block(block, self, self);
    memcpy((uint8_t *)&current_dir_entry, (uint8_t *)&inode_buffer, sizeof(inode_entry_t));
}

void print_superblock()
{
    kprintf("[FS] File system current informatin:      \n");
    kprintf("     magic number : 0x%x                  \n", superblock->magic);
    kprintf("     start sector : 0x%x                  \n", superblock->start_sector);
    kprintf("     file system size : 0x%x              \n", superblock->fs_size);
    kprintf("     block size : 0x%x                    \n", superblock->block_size);
    kprintf("     block bitmap offset : 0x%x           \n", superblock->block_bmp_offset);
    kprintf("     inode bitmap offset : 0x%x           \n", superblock->inode_bmp_offset);
    kprintf("     inode offset : 0x%x                  \n", superblock->inode_offset);
    kprintf("     data offset : 0x%x                   \n", superblock->data_offset);
    kprintf("     total inodes : %d                    \n", superblock->total_inode_num);
    kprintf("     free inodes : %d                     \n", superblock->free_inode_num);
    kprintf("     total blocks : %d                    \n", superblock->total_block_num);
    kprintf("     free blocks : %d                     \n", superblock->free_block_num);
    kprintf("     inode entry size : %d                \n", superblock->inode_size);
    kprintf("     dir entry size : %d                  \n", superblock->dir_size);
}

int readdir(char *name)
{
    return 0;
}

int mkdir(char *name)
{
    return 0;
}

int rmdir(char *name)
{
    return 0;
}

int mknod(char *name)
{
    
    return 0; // failure
}

int open(char *name, uint32_t access)
{
    
    return -1; // open failure
}

int write(uint32_t fd, char *buff, uint32_t size)
{
    
    return 0;
}

int read(uint32_t fd, char *buff, uint32_t size)
{
    
    return 0;
}

int close(uint32_t fd)
{
    
    return 0;
}

 
int cat(char *name)
{
    
    return -1; // open failure
}

void do_mkfs()
{
    int i;
    kprintf("[FS] Starting initialize file system!     \n");
    bzero(fds, NUM_FD * sizeof(fd_t));
    // bzero(inode_buffer, BLOCK_SIZE);
    // bzero(data_block_buffer, BLOCK_SIZE);
    // bzero(dir_entry_block_buffer, BLOCK_SIZE);

    // init superblock
    kprintf("[FS] Setting superblock...                \n");
    init_superblock();

    // init block bitmap
    kprintf("[FS] Setting block bitmap...              \n");
    init_blockbmp();

    // init inode bitmap
    kprintf("[FS] Setting inode bitmap...              \n");
    init_inodebmp();

    // init root directory
    kprintf("[FS] Setting root dir...                  \n");
    init_rootdir();

    kprintf("[FS] Initializing file system finished!   \n");
    print_superblock();
}

void do_statfs()
{
    read_superblock();
    if(superblock->magic != KFS_MAGIC){
        kprintf("[ERROR] No File System!\n");
        return;
    }
    print_superblock();
}

void init_fs()
{
    read_superblock();
    if (superblock->magic == KFS_MAGIC) {
        // sync_from_disk_inode(0, root_inode_ptr);
        // memcpy((uint8_t *)&current_dir_entry, (uint8_t *)&root_inode, sizeof(inode_entry_t));
        kprintf("[FS] File system has existed in disk!      \n");
        do_statfs();
    }
    else {
        do_mkfs();
    }
}
