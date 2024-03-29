#include "fs.h"
#include "string.h"
#include "common.h"
#include "screen.h"
#include "sched.h"

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
// static super_block_t *superblock = (super_block_t *)0xffffffffaf000000;

static fd_t fds[NUM_FD];
static num_open_files = 0;
static inode_entry_t current_dir_entry;
static current_dir_id = 0;

void read_superblock()
{
    sd_card_read(superblock, OFFSET_FS, SECTOR_SIZE);
}

void write_superblock()
{
    // printk("sd_card_write\n\r");
    sd_card_write(superblock, OFFSET_FS, SECTOR_SIZE);
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
    // kprintf("read id = %d inode.fnum = %d\n\r", id, inode_buffer.fnum);
}

void write_inode(uint32_t id)
{
    uint32_t sector = id / (SECTOR_SIZE / sizeof(inode_entry_t));
    uint32_t part = id % (SECTOR_SIZE / sizeof(inode_entry_t));
    sd_card_read((void *)BUFFER, superblock->inode_offset + sector * SECTOR_SIZE, SECTOR_SIZE);
    memcpy((uint8_t *)(BUFFER + part * sizeof(inode_entry_t)), (uint8_t *)&inode_buffer, sizeof(inode_entry_t));
    sd_card_write((void *)BUFFER, superblock->inode_offset + sector * SECTOR_SIZE, SECTOR_SIZE);
    // kprintf("write id = %d inode.fnum = %d\n\r", id, inode_buffer.fnum);
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
    for (i = 0; i < INODE_BITMAP_SIZE; i++)
    {
        if (inodebmp_buffer[i] != 0xff)
        {
            break;
        }
    }
    uint8_t temp = inodebmp_buffer[i];
    for (j = 0; j < 8; j++)
    {
        if ((temp & 0x01) == 0)
        {
            break;
        }
        else
        {
            temp = temp >> 1;
        }
    }
    temp = 0x01 << j;
    inodebmp_buffer[i] |= temp;
    write_inodebmp();
    superblock->free_inode_num -= 1;
    write_superblock();
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
    superblock->free_inode_num += 1;
    write_superblock();
}

uint32_t alloc_block()
{
    read_blockbmp();
    int i, j;
    for (i = 0; i < BLOCK_BITMAP_SIZE; i++)
    {
        if (blockbmp_buffer[i] != 0xff)
        {
            break;
        }
    }
    uint8_t temp = blockbmp_buffer[i];
    for (j = 0; j < 8; j++)
    {
        if ((temp & 0x01) == 0)
        {
            break;
        }
        else
        {
            temp = temp >> 1;
        }
    }
    temp = 0x01 << j;
    blockbmp_buffer[i] |= temp;
    write_blockbmp();
    superblock->free_block_num -= 1;
    write_superblock();
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
    superblock->free_block_num += 1;
    write_superblock();
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
    superblock->total_block_num = NUM_BLOCK;
    superblock->free_block_num = NUM_BLOCK - INIT_USED_BLOCK;
    superblock->inode_size = sizeof(inode_entry_t);
    superblock->dir_size = sizeof(dir_entry_t);
    // printk("write_superblock\n\r");
    write_superblock();
}

void init_blockbmp()
{
    bzero(blockbmp_buffer, BLOCK_BITMAP_SIZE);
    int i, j;
    for (i = 0; i < INIT_USED_BLOCK / 8; i++)
    {
        blockbmp_buffer[i] = 0xff;
    }
    for (j = 0; j < INIT_USED_BLOCK % 8; j++)
    {
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
    if (type == TYPE_DIR || type == TYPE_ROOT)
    {
        inode_buffer.fsize = 2 * sizeof(dir_entry_t);
    }
    else
    {
        inode_buffer.fsize = 0;
    }
    inode_buffer.fnum = 0;
    inode_buffer.timestamp = get_timer();
    inode_buffer.direct_table[0] = block;
    int i;
    for (i = 1; i < MAX_DIRECT_NUM; i++)
    {
        inode_buffer.direct_table[i] = 0;
    }
    inode_buffer.indirect_1_ptr = 0;
    inode_buffer.indirect_2_ptr = 0;
    inode_buffer.indirect_3_ptr = 0;

    write_inode(inode_buffer.id);

    // if (inode_buffer.type != TYPE_ROOT) {
    //     read_inode(father);
    //     uint32_t fnum = ++(inode_buffer.fnum);
    //     write_inode(father);

    //     uint32_t block = inode_buffer.direct_table[0];
    //     read_block(block);
    //     dir_entry_t* dir;
    //     int i;
    //     for (i = 0; i < fnum; i++) {
    //         dir = (dir_entry_t*)(BUFFER + i * sizeof(dir_entry_t));
    //         if (dir->name[0] == '\0') break;
    //     }
    //     // while (inst_path[0][j] != '\0') {
    //     //     dir->name[j] = inst_path[0][j];
    //     //     j++;
    //     // }
    //     dir->name[i] = '\0';
    //     dir->id = self;
    //     dir->type = type;

    //     write_block(block);
    // }
}

// init directory block
void init_dir_block(uint32_t block, uint16_t father, uint16_t self)
{
    read_block(block);

    dir_entry_t *dir = (dir_entry_t *)BUFFER;
    dir->name[0] = '.';
    dir->name[1] = '.';
    dir->name[2] = '\0';
    dir->id = father;
    dir->type = TYPE_FATHER;
    dir->mode = O_RDWR;

    dir = (dir_entry_t *)(BUFFER + sizeof(dir_entry_t));
    dir->name[0] = '.';
    dir->name[1] = '\0';
    dir->id = self;
    dir->type = TYPE_SELF;
    dir->mode = O_RDWR;

    write_block(block);
}

void init_rootdir()
{
    uint32_t self = alloc_inode();
    if (self != ROOT_ID)
    {
        kprintf("[ERROR] ROOT DIR INODE NUMBER = %d, NOT ZERO\n\r", self);
    }
    uint32_t block = alloc_block();
    init_inode(block, self, self, O_RDWR, TYPE_ROOT);
    init_dir_block(block, self, self);
    memcpy((uint8_t *)&current_dir_entry, (uint8_t *)&inode_buffer, sizeof(inode_entry_t));
}

void print_superblock()
{
    int temp_x = screen_cursor_x;
    int temp_y = screen_cursor_y;
    // do_clear();
    vt100_move_cursor(1, 3);
    printk("[FS] File system current informatin:      \n\r");
    printk("     magic number : 0x%x, start sector : 0x%x\n\r", superblock->magic, superblock->start_sector);
    printk("     file system size : 0x%x, block size : 0x%x\n\r", superblock->fs_size, superblock->block_size);
    printk("     block bitmap offset : 0x%x           \n\r", superblock->block_bmp_offset);
    printk("     inode bitmap offset : 0x%x           \n\r", superblock->inode_bmp_offset);
    printk("     inode offset : 0x%x                  \n\r", superblock->inode_offset);
    printk("     data offset : 0x%x                   \n\r", superblock->data_offset);
    printk("     total inodes : %d, free inodes : %d  \n\r", superblock->total_inode_num, superblock->free_inode_num);
    printk("     total blocks : %d, free blocks : %d  \n\r", superblock->total_block_num, superblock->free_block_num);
    printk("     inode entry size : %d, dir entry size : %d\n\r", superblock->inode_size, superblock->dir_size);
    screen_cursor_x = temp_x;
    screen_cursor_y = temp_y;
}

void save_current_dir()
{
    current_dir_id = current_dir_entry.id;
}

void restore_current_dir()
{
    read_inode(current_dir_id);
    memcpy((uint8_t *)&current_dir_entry, (uint8_t *)&inode_buffer, sizeof(inode_entry_t));
}

int is_name_in_dir(char *name)
{
    read_block(current_dir_entry.direct_table[0]);
    int i;
    dir_entry_t *dir;
    for (i = 0; i < current_dir_entry.fnum; i++)
    {
        dir = (dir_entry_t *)(BUFFER + (i + 2) * sizeof(dir_entry_t));
        if (!strcmp(dir->name, name))
        {
            break;
        }
    }
    if (i == current_dir_entry.fnum)
    {
        kprintf("No such file or dir: %s\n", name);
        return -1; // failure
    }
    else
    {
        // return dir->id;
        return i;
    }
}

int do_enterdir(char *name)
{
    int id;
    // printk("In cd...\n\r");
    if (name[strlen(name) - 1] == '/' && strlen(name) != 1)
    {
        name[strlen(name) - 1] = '\0';
    }
    char name_buf[10];
    while (strlen(name))
    {
        // printk("name len: %d, %s\n\r", strlen(name), name);
        if (name[0] == '/')
        {
            id = ROOT_ID;
            strcpy(name_buf, "root");
            name++;
        }
        else if (name[0] == '.' && name[1] == '.')
        {
            if (name[2] == '/' || name[2] == '\0')
            {
                read_block(current_dir_entry.direct_table[0]);
                dir_entry_t *dir = (dir_entry_t *)BUFFER; // ..
                id = dir->id;
                if (name[2] == '/')
                {
                    name += 3;
                }
                else
                {
                    name += 2;
                }
                strcpy(name_buf, "..");
            }
            else
            {
                kprintf("Invalid path: %s\n", name);
                return -1;
            }
        }
        else if (name[0] == '.')
        {
            if (name[1] == '/' || name[1] == '\0')
            {
                if (name[1] == '/')
                {
                    name += 2;
                }
                else
                {
                    // '\0'
                    name += 1;
                    break;
                }
            }
            else
            {
                kprintf("Invalid path: %s", name);
                return -1;
            }
            continue;
        }
        else
        {
            int j;
            for (j = 0; j < strlen(name) && name[j] != '/'; j++)
            {
                name_buf[j] = name[j];
            }
            name_buf[j] = '\0';
            // printk("name buf: %s\n\r", name_buf);
            int i = is_name_in_dir(name_buf);
            if (i == -1)
            {
                kprintf("Dir: %s not found...\n", name);
                // do_mkdir(name_buf);
                // i = is_name_in_dir(name_buf);
                return -1;
            }
            dir_entry_t *dir = (dir_entry_t *)(BUFFER + (i + 2) * sizeof(dir_entry_t));
            id = dir->id;
            if (name[j] == '\0')
            {
                name += j;
            }
            else
            {
                name += j + 1;
            }
        }
        kprintf("Opening dir: %s (id = %d)...\n", name_buf, id);
        read_inode(id);
        memcpy((uint8_t *)&current_dir_entry, (uint8_t *)&inode_buffer, sizeof(inode_entry_t));
    }
    return 0;
}

int do_readdir(char *name)
{
    // TODO: name
    save_current_dir();
    do_enterdir(name);
    read_block(current_dir_entry.direct_table[0]);
    int i;
    dir_entry_t *dir = (dir_entry_t *)(BUFFER + 2 * sizeof(dir_entry_t));
    kprintf(". ..\n");
    if (current_dir_entry.fnum)
    {
        kprintf("  Name \t TYPE \t Mode\n");
        // kprintf("current_dir_entry.fnum = %d\n\r", current_dir_entry.fnum);
    }
    for (i = 0; i < current_dir_entry.fnum; i++)
    {
        dir_entry_t *dir = (dir_entry_t *)(BUFFER + (i + 2) * sizeof(dir_entry_t));
        kprintf("  %s    %s    %s\n",
                dir->name,
                (dir->type == TYPE_DIR) ? "DIR" : "FILE",
                (dir->mode == O_RDWR) ? "O_RDWR" : (dir->mode == O_WRONLY) ? "O_WRONLY" : "O_RDONLY");
    }
    restore_current_dir();
    return 0;
}

int do_mkdir(char *name)
{
    // TODO: check name
    kprintf("Creating dir: %s...\n", name);

    uint32_t id = alloc_inode();
    uint32_t block = alloc_block();
    init_inode(block, current_dir_entry.id, id, O_RDWR, TYPE_DIR);
    init_dir_block(block, current_dir_entry.id, id);

    // modify current dir block
    read_block(current_dir_entry.direct_table[0]); // TODO: more dir
    dir_entry_t *dir = (dir_entry_t *)(BUFFER + (current_dir_entry.fnum + 2) * sizeof(dir_entry_t));
    dir->id = id;
    dir->type = TYPE_DIR;
    dir->mode = O_RDWR;
    strcpy(dir->name, name);
    write_block(current_dir_entry.direct_table[0]);

    // modify current dir entry
    current_dir_entry.fsize += sizeof(dir_entry_t);
    current_dir_entry.fnum += 1;
    // printk("current id = %d inode.fnum = %d\n\r", current_dir_entry.id, current_dir_entry.fnum);
    current_dir_entry.timestamp = get_timer();
    memcpy((uint8_t *)&inode_buffer, (uint8_t *)&current_dir_entry, sizeof(inode_entry_t));
    write_inode(current_dir_entry.id);

    // return -1; // failure
    return 0;
}

int do_rmdir(char *name)
{
    // TODO: check name
    kprintf("In rmdir...\n");
    dir_entry_t *dir;
    int i = is_name_in_dir(name);
    dir = (dir_entry_t *)(BUFFER + (i + 2) * sizeof(dir_entry_t));
    if (i == -1)
    {
        // kprintf("No such file or dir: %s\n", name);
        return -1; // failure
    }
    else
    {
        kprintf("Removing dir: %s...\n", name);
        // modify current dir inode
        current_dir_entry.fsize -= sizeof(dir_entry_t);
        current_dir_entry.fnum -= 1;
        current_dir_entry.timestamp = get_timer();
        memcpy((uint8_t *)&inode_buffer, (uint8_t *)&current_dir_entry, sizeof(inode_entry_t));
        write_inode(current_dir_entry.id);

        // free rm dir inode and block
        read_inode(dir->id);
        free_block(inode_buffer.direct_table[0]); // TODO: more block
        free_inode(dir->id);

        // modify current dir block
        read_block(current_dir_entry.direct_table[0]);
        bzero(dir, sizeof(dir_entry_t));
        if (i != current_dir_entry.fnum)
        {
            memcpy((char *)dir, (char *)(BUFFER + (current_dir_entry.fnum + 2) * sizeof(dir_entry_t)), sizeof(dir_entry_t));
        }
        write_block(current_dir_entry.direct_table[0]);
    }
    return 0;
}

int do_mknod(char *name)
{
    int id = is_name_in_dir(name);
    if (id == -1)
    {
        kprintf("File: %s not found, creating...\n", name);
        id = init_file(name, O_RDWR);
        return 0;
    }
    else
    {
        kprintf("File: %s has existed\n", name);
        return -1; // failure
    }
}

int init_file(char *name, uint32_t access)
{
    // TODO: check name
    kprintf("Initializing file: %s...\n", name);

    uint32_t id = alloc_inode();
    uint32_t block = alloc_block();
    init_inode(block, current_dir_entry.id, id, O_RDWR, TYPE_FILE);

    // modify current dir block
    read_block(current_dir_entry.direct_table[0]);
    dir_entry_t *dir = (dir_entry_t *)(BUFFER + (current_dir_entry.fnum + 2) * sizeof(dir_entry_t));
    dir->id = id;
    dir->type = TYPE_FILE;
    dir->mode = access;
    strcpy(dir->name, name);
    write_block(current_dir_entry.direct_table[0]);

    // modify current dir entry
    current_dir_entry.fsize += sizeof(dir_entry_t);
    current_dir_entry.fnum += 1;
    // printk("current id = %d inode.fnum = %d\n\r", current_dir_entry.id, current_dir_entry.fnum);
    current_dir_entry.timestamp = get_timer();
    memcpy((uint8_t *)&inode_buffer, (uint8_t *)&current_dir_entry, sizeof(inode_entry_t));
    write_inode(current_dir_entry.id);
    return id;
}

int do_open(char *name, uint32_t access)
{
    int id = is_name_in_dir(name);
    if (id == -1)
    {
        kprintf("File: %s not found, creating...\n", name);
        id = init_file(name, access);
    }
    else
    {
        dir_entry_t *dir = (dir_entry_t *)(BUFFER + (id + 2) * sizeof(dir_entry_t));
        id = dir->id;
    }
    kprintf("Opening file: %s (id = %d)...\n", name, id);
    read_inode(id);
    fds[num_open_files].inode_id = id;
    // fds[num_open_files].fsize = inode_buffer.fsize;
    fds[num_open_files].mode = inode_buffer.mode;
    fds[num_open_files].r_offset = 0;
    fds[num_open_files].w_offset = 0;
    return num_open_files++;
    // return -1; // open failure
}

int do_write(uint32_t fd, char *buff, uint32_t size)
{
    // kprintf("Writing to %d size = %d: ", fds[fd].inode_id, size);
    // kprintf("%s", buff);
    read_inode(fds[fd].inode_id);

    // read_block(inode_buffer.direct_table[0]);
    // // kprintf(" block id = %d\n", inode_buffer.direct_table[0]);
    // char *p_block = (char *)(BUFFER + fds[fd].w_offset);
    // memcpy(p_block, buff, size);
    // write_block(inode_buffer.direct_table[0]);
    int block_id;
    int write_offset;
    int write_size;
    uint32_t begin_block;
    char * p_block;
    uint32_t * id_block;
    while (size > 0)
    {
        begin_block = fds[fd].w_offset / BLOCK_SIZE;
        if (fds[fd].w_offset <= MAX_DIRECT_NUM * BLOCK_SIZE)
        {
            block_id = inode_buffer.direct_table[begin_block];
            if (block_id == 0) {
                block_id = alloc_block();
                inode_buffer.direct_table[begin_block] = block_id;
            }
            read_block(block_id);
        }
        else if (fds[fd].w_offset <= MAX_DIRECT_NUM * BLOCK_SIZE + (BLOCK_SIZE / sizeof(uint32_t)) * BLOCK_SIZE)
        {
            block_id = inode_buffer.indirect_1_ptr;
            if (block_id == 0) {
                block_id = alloc_block();
                inode_buffer.indirect_1_ptr = block_id;
                read_block(block_id);
                bzero((void *)BUFFER, BLOCK_SIZE);
                write_block(block_id);
            }
            // kprintf("inode_buffer.indirect_1_ptr\n");
            read_block(block_id);
            id_block = (uint32_t *)(BUFFER + (begin_block - MAX_DIRECT_NUM) * sizeof(uint32_t));
            // if (*id_block == 0) {
                int temp = alloc_block();
                *id_block = temp;
                // kprintf("alloc_block: %d\n", temp);
                write_block(block_id);
                block_id = temp;
            // }
            // else {
            //     block_id = *id_block;
            // }
            // kprintf("block id: %d\n", block_id);
            // kprintf("inode_buffer.indirect_1_ptr done\n");
            read_block(block_id);
        }
        else if (fds[fd].w_offset <= MAX_DIRECT_NUM * BLOCK_SIZE + (BLOCK_SIZE / sizeof(uint32_t)) * BLOCK_SIZE + (BLOCK_SIZE / sizeof(uint32_t)) * (BLOCK_SIZE / sizeof(uint32_t)) * BLOCK_SIZE)
        {
            block_id = inode_buffer.indirect_2_ptr;
            if (block_id == 0) {
                block_id = alloc_block();
                inode_buffer.indirect_2_ptr = block_id;
                read_block(block_id);
                bzero((void *)BUFFER, BLOCK_SIZE);
                write_block(block_id);
            }
            read_block(block_id);
            id_block = (uint32_t *)(BUFFER + (begin_block - MAX_DIRECT_NUM - (BLOCK_SIZE / sizeof(uint32_t))) / (BLOCK_SIZE / sizeof(uint32_t))  * sizeof(uint32_t));
            if (*id_block == 0) {
                int temp = alloc_block();
                *id_block = temp;
                write_block(block_id);
                block_id = temp;
            }
            else {
                block_id = *id_block;
            }
            read_block(block_id);
            id_block = (uint32_t *)(BUFFER + ((begin_block - MAX_DIRECT_NUM - (BLOCK_SIZE / sizeof(uint32_t))) % (BLOCK_SIZE / sizeof(uint32_t))) * sizeof(uint32_t));
            if (*id_block == 0) {
                int temp = alloc_block();
                *id_block = temp;
                write_block(block_id);
                block_id = temp;
            }
            else {
                block_id = *id_block;
            }
            read_block(block_id);
        }
        else
        {
            kprintf("Too large size. Not supported.\n");
        }
        write_offset = fds[fd].w_offset - begin_block * BLOCK_SIZE;
        write_size = ((BLOCK_SIZE - write_offset) > size) ? size : (BLOCK_SIZE - write_offset);
        // kprintf("loaded block: %d, w_offset: %d, write_offset: %d, write_size: %d\t", block_id, fds[fd].w_offset, write_offset, write_size);
        p_block = (char *)(BUFFER + write_offset);
        memcpy(p_block, buff, write_size);
        write_block(block_id);
        // kprintf("memcpy(buff, p_block, read_size) done.\n");
        buff += write_size;
        fds[fd].w_offset += write_size;
        if (inode_buffer.fsize < fds[fd].w_offset)
        {
            inode_buffer.fsize = fds[fd].w_offset;
        }
        size -= write_size;
    }

    // kprintf("File size: %d\n", inode_buffer.fsize);
    inode_buffer.timestamp = get_timer();
    write_inode(fds[fd].inode_id);
    return 0;
}

int do_read(uint32_t fd, char *buff, uint32_t size)
{
    read_inode(fds[fd].inode_id);
    int end = fds[fd].r_offset + size;
    if (end > inode_buffer.fsize)
    {
        kprintf("Read offset(0x%x) is out of fsize(0x%x)!\n", end, inode_buffer.fsize);
        return -1; // failure
    }
    // if (size > BLOCK_SIZE)
    // {
    //     kprintf("Read size(0x%x) is to large.\n", size);
    // }

    // read_block(inode_buffer.direct_table[0]);
    // char * p_block = (char *)(BUFFER + fds[fd].r_offset);
    // memcpy(buff, p_block, size);

    int block_id;
    int read_offset;
    int read_size;
    uint32_t begin_block;
    char * p_block;
    uint32_t * id_block;
    while (size > 0)
    {
        begin_block = fds[fd].r_offset / BLOCK_SIZE;
        if (fds[fd].r_offset <= MAX_DIRECT_NUM * BLOCK_SIZE)
        {
            block_id = inode_buffer.direct_table[begin_block];
            read_block(block_id);
        }
        else if (fds[fd].r_offset <= MAX_DIRECT_NUM * BLOCK_SIZE + (BLOCK_SIZE / sizeof(uint32_t)) * BLOCK_SIZE)
        {
            block_id = inode_buffer.indirect_1_ptr;
            read_block(block_id);
            id_block = (uint32_t *)(BUFFER + (begin_block - MAX_DIRECT_NUM) * sizeof(uint32_t));
            block_id = *id_block;
            read_block(block_id);
        }
        else if (fds[fd].r_offset <= MAX_DIRECT_NUM * BLOCK_SIZE + (BLOCK_SIZE / sizeof(uint32_t)) * BLOCK_SIZE + (BLOCK_SIZE / sizeof(uint32_t)) * (BLOCK_SIZE / sizeof(uint32_t)) * BLOCK_SIZE)
        {
            block_id = inode_buffer.indirect_2_ptr;
            read_block(block_id);
            id_block = (uint32_t *)(BUFFER + (begin_block - MAX_DIRECT_NUM - (BLOCK_SIZE / sizeof(uint32_t))) / (BLOCK_SIZE / sizeof(uint32_t)) * sizeof(uint32_t));
            block_id = *id_block;
            read_block(block_id);
            id_block = (uint32_t *)(BUFFER + ((begin_block - MAX_DIRECT_NUM - (BLOCK_SIZE / sizeof(uint32_t))) % (BLOCK_SIZE / sizeof(uint32_t))) * sizeof(uint32_t));
            block_id = *id_block;
            read_block(block_id);
        }
        else
        {
            kprintf("Too large size. Not supported.\n");
        }
        read_offset = fds[fd].r_offset - begin_block * BLOCK_SIZE;
        read_size = ((BLOCK_SIZE - read_offset) > size) ? size : (BLOCK_SIZE - read_offset);
        // kprintf("loaded block: %d, r_offset: %d, read_offset: %d, read_size: %d\t", block_id, fds[fd].r_offset, read_offset, read_size);
        p_block = (char *)(BUFFER + read_offset);
        memcpy(buff, p_block, read_size);
        // kprintf("memcpy(buff, p_block, read_size) done.\n");
        buff += read_size;
        fds[fd].r_offset += read_size;
        size -= read_size;
    }

    // fds[fd].r_offset += size;
    inode_buffer.timestamp = get_timer();
    write_inode(fds[fd].inode_id);
    return 0;
}

int do_close(uint32_t fd)
{
    num_open_files--;
    if (fd == num_open_files)
    {
        bzero(&fds[fd], sizeof(fd_t));
    }
    else
    {
        memcpy((char *)&fds[fd], (char *)&fds[num_open_files], sizeof(fd_t));
    }
    return 0;
}

int do_cat(char *name)
{
    int id = is_name_in_dir(name);
    if (id == -1)
    {
        kprintf("File: %s not found\n", name);
        return -1; // open failure
    }
    else
    {
        dir_entry_t *dir = (dir_entry_t *)(BUFFER + (id + 2) * sizeof(dir_entry_t));
        id = dir->id;
        if (dir->type == TYPE_SLINK) {
            read_inode(id);
            read_block(inode_buffer.direct_table[0]);
            uint32_t * p_data = (uint32_t *)BUFFER;
            id = *p_data;
        }
        else if (dir->type != TYPE_FILE)
        {
            kprintf("%s is not a file, but a dir\n", name);
            return -1; // failure
        }
    }
    kprintf("Opening file: %s (id = %d)...\n", name, id);
    read_inode(id);
    read_block(inode_buffer.direct_table[0]);
    kprintf("File size: %d\n", inode_buffer.fsize);
    char *data_block = (char *)BUFFER;
    int i;
    for (i = 0; i < inode_buffer.fsize; i++)
    {
        kprintf("%c", data_block[i]);
        if (i > SECTOR_SIZE) {
            break;
        }
    }
    return 0;
}

int do_link(char *src, char *dest, uint32_t soft)
{
    int src_id = is_name_in_dir(src);
    if (src_id == -1)
    {
        kprintf("File: %s not found\n", src);
        return -1; // open failure
    }
    read_block(current_dir_entry.direct_table[0]);
    dir_entry_t *src_dir = (dir_entry_t *)(BUFFER + (src_id + 2) * sizeof(dir_entry_t));
    src_id = src_dir->id;
    if (soft) {
        uint32_t id = alloc_inode();
        uint32_t block = alloc_block();
        init_inode(block, current_dir_entry.id, id, O_RDWR, TYPE_FILE);

        // modify current dir block
        dir_entry_t *dir = (dir_entry_t *)(BUFFER + (current_dir_entry.fnum + 2) * sizeof(dir_entry_t));
        read_block(current_dir_entry.direct_table[0]);
        dir->id = id;
        dir->type = TYPE_SLINK;
        dir->mode = src_dir->mode;
        strcpy(dir->name, dest);
        write_block(current_dir_entry.direct_table[0]);

        // modify current dir entry
        current_dir_entry.fsize += sizeof(dir_entry_t);
        current_dir_entry.fnum += 1;
        // printk("current id = %d inode.fnum = %d\n\r", current_dir_entry.id, current_dir_entry.fnum);
        current_dir_entry.timestamp = get_timer();
        memcpy((uint8_t *)&inode_buffer, (uint8_t *)&current_dir_entry, sizeof(inode_entry_t));
        write_inode(current_dir_entry.id);
        
        read_block(block);
        uint32_t * p_data = (uint32_t *)BUFFER;
        * p_data = src_id;
        write_block(block);
        return 0;
    }
    else {
        // modify current dir block
        dir_entry_t *dir = (dir_entry_t *)(BUFFER + (current_dir_entry.fnum + 2) * sizeof(dir_entry_t));
        dir->id = src_dir->id;
        dir->type = src_dir->type;
        dir->mode = src_dir->mode;
        strcpy(dir->name, dest);
        write_block(current_dir_entry.direct_table[0]);

        // modify current dir entry
        current_dir_entry.fsize += sizeof(dir_entry_t);
        current_dir_entry.fnum += 1;
        current_dir_entry.links_cnt += 1;
        // printk("current id = %d inode.fnum = %d\n\r", current_dir_entry.id, current_dir_entry.fnum);
        current_dir_entry.timestamp = get_timer();
        memcpy((uint8_t *)&inode_buffer, (uint8_t *)&current_dir_entry, sizeof(inode_entry_t));
        write_inode(current_dir_entry.id);
        return 0;
    }
}

void do_mkfs()
{
    int temp_x = screen_cursor_x;
    int temp_y = screen_cursor_y;
    // do_clear();
    vt100_move_cursor(1, 3);
    printk("[FS] Starting initialize file system!     \n\r");
    bzero(fds, NUM_FD * sizeof(fd_t));
    // bzero(inode_buffer, BLOCK_SIZE);
    // bzero(data_block_buffer, BLOCK_SIZE);
    // bzero(dir_entry_block_buffer, BLOCK_SIZE);

    // init superblock
    printk("[FS] Setting superblock...                \n\r");
    init_superblock();

    // init block bitmap
    printk("[FS] Setting block bitmap...              \n\r");
    init_blockbmp();

    // init inode bitmap
    printk("[FS] Setting inode bitmap...              \n\r");
    init_inodebmp();

    // init root directory
    printk("[FS] Setting root dir...                  \n\r");
    init_rootdir();

    printk("[FS] Initializing file system finished!   \n\r");
    print_superblock();
    screen_cursor_x = temp_x;
    screen_cursor_y = temp_y;
}

void do_statfs()
{
    read_superblock();
    if (superblock->magic != KFS_MAGIC)
    {
        kprintf("[ERROR] No File System!\n\r");
        return;
    }
    print_superblock();
}

void init_fs()
{
    read_superblock();
    if (superblock->magic == KFS_MAGIC)
    {
        printk("[FS] File system has existed in disk!      \n\r");
        do_statfs();

        // load root directory: inode
        read_inode(ROOT_ID);
        // point to root directory
        memcpy((uint8_t *)&current_dir_entry, (uint8_t *)&inode_buffer, sizeof(inode_entry_t));
        // printk("current_dir_entry.fnum = %d\n\r", current_dir_entry.fnum);

        read_inodebmp();
        read_blockbmp();
    }
    else
    {
        printk("[FS] No File system!       \n\r");
        do_mkfs();
    }
}
