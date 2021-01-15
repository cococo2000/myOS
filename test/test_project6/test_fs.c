#include "test_fs.h"
#include "fs.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"

static char buff[4096];

void test_fs(void)
{
    int i, j;
    int fd = sys_fopen("1.txt", O_RDWR);

    // write 'hello world!' * 10
    for (i = 0; i < 10; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }

    // read
    for (i = 0; i < 10; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }

    sys_close(fd);
    sys_exit();
}

void test_big_file(void)
{
    int i, j;
    int fd = sys_fopen("1.txt", O_RDWR);

    // write 'hello world!' * 10
    for (i = 0; i < 1000000; i++)
    {
        sys_fwrite(fd, "hello world!\n", 4096);
    }

    // read
    for (i = 0; i < 1000000; i++)
    {
        sys_fread(fd, buff, 4096);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }

    sys_close(fd);
    sys_exit();
}