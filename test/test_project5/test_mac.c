#include "mac.h"
#include "irq.h"
#include "type.h"
#include "screen.h"
#include "syscall.h"
#include "sched.h"
#include "test5.h"

#define SHM_KEY 1

#define HDR_OFFSET 54
#define SHMP_KEY 0x42
#define MAGIC 0xbeefbeefbeefbeeflu
#define FIFO_BUF_MAX 2048

#define MAX_RECV_CNT 64
char recv_buffer1[MAX_RECV_CNT * PSIZE * 4];
char recv_buffer2[MAX_RECV_CNT * PSIZE * 4];
uint64_t recv_length[MAX_RECV_CNT] = {0};

const char response[] = "Response: ";

/* this should not exceed a page */
typedef struct echo_shm_vars
{
    uint64_t magic_number;
    uint64_t available;
    char fifo_buffer[FIFO_BUF_MAX];
} echo_shm_vars_t;

void shm_read(char *shmbuf, uint64_t *_available, char *buf, uint64_t size)
{
    while (size > 0)
    {
        while (_available != 1)
            ;

        int sz = size > FIFO_BUF_MAX ? FIFO_BUF_MAX : size;
        memcpy(buf, shmbuf, sz);
        size -= sz;

        _available = 0;
    }
}

void shm_write(char *shmbuf, uint64_t *_available, char *buf, uint64_t size)
{
    while (size > 0)
    {
        while (_available != 0)
            ;

        int sz = size > FIFO_BUF_MAX ? FIFO_BUF_MAX : size;
        memcpy(shmbuf, buf, sz);
        size -= sz;

        _available = 1;
    }
}

static uint32_t printf_recv_buffer(uint64_t recv_buffer)
{
    uint32_t i, flag, n;
    flag = 0;
    n = 0;
    for (i = 0; i < PSIZE * PNUM; i++)
    {
        if ((*((uint32_t *)recv_buffer + i) != 0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f))
        {
            if (*((uint32_t *)recv_buffer + i) != 0xffffff)
            {
                printf(" %x ", *((uint32_t *)recv_buffer + i));

                if (n % 10 == 0 && n != 0)
                {
                    printf(" \n");
                }
                n++;
                flag = 1;
            }
        }
    }
    return flag;
}

void mac_send_task()
{
    int mode = 0;
    int size = PNUM;

    int send_num = 0;
    int i;

    int resp_len = strlen(response);
    int print_location = 10;
    sys_move_cursor(1, print_location);
    printf("[ECHO SEND SIDE]\n");
#ifdef P5_C_CORE
    char *cur = recv_buffer;
    uint64_t shmid;
    shmid = shmget(SHM_KEY);
    echo_shm_vars_t *vars = (echo_shm_vars_t *)shmat(shmid);
    if (vars == NULL)
    {
        sys_move_cursor(1, 1);
        printf("shmpageget failed!\n");
        return -1;
    }

    shm_read(vars->fifo_buffer, &vars->available, recv_buffer, size * sizeof(PSIZE));
    shm_read(vars->fifo_buffer, &vars->available, recv_length, size * sizeof(uint64_t));
    for (i = 0; i < size; ++i)
    {
        sys_move_cursor(1, print_location);
        printf("No.%d packet, recv_length[i] = %d ...\n", i, recv_length[i]);
        memcpy(cur + HDR_OFFSET, response, resp_len);

        sys_net_send(cur, recv_length[i], 1);
        send_num += 1;
        sys_move_cursor(1, print_location + 1);
        printf("[ECHO TASK] Echo no.%d packets ...\n", i);
        cur += recv_length[i];
    }

    shmpagedt((void *)vars);
#else
    uint32_t buffer[PSIZE] = {0xffffffff, 0x5500ffff, 0xf77db57d, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0004e914, 0x0000, 0x005e0001, 0x2300fb00, 0x84b7f28b, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0801e914, 0x0000};
    for (i = 0; i < size; ++i)
    {
        sys_move_cursor(1, print_location);
        printf("No.%d packet, recv_length[i] = %d ...\n", i, PSIZE);

        sys_net_send(buffer, PSIZE, PNUM);
        // do_net_send((uint64_t)buffer, PSIZE, PNUM);
        send_num += 1;
        sys_move_cursor(1, print_location + 1);
        printf("[ECHO TASK] Echo no.%d packets ...\n", i);
    }
#endif
    sys_exit();
}

void mac_recv_task()
{
    int mode = 0;
    int size = PNUM;
#ifdef P5_C_CORE
    uint64_t shmid;
    shmid = shmget(SHM_KEY);
    echo_shm_vars_t *vars = (echo_shm_vars_t *)shmat(shmid);
    if (vars == NULL)
    {
        sys_move_cursor(1, 1);
        printf("shmpageget failed!\n");
        return -1;
    }

    sys_move_cursor(1, 1);

    vars->available = 0;

    sys_move_cursor(1, 1);
    printf("[ECHO TASK] start recv(%d):                    \n", size);

    int ret = sys_net_recv(recv_buffer, size * PSIZE, size, recv_length);
    shm_write(vars->fifo_buffer, &vars->available, recv_buffer, size * PSIZE);
    shm_write(vars->fifo_buffer, &vars->available, recv_length, size * sizeof(uint64_t));

    shmdt((void *)vars);
#else
    sys_move_cursor(1, 1);
    printf("[ECHO TASK] start recv(%d):                    \n", size * 2);

    int ret = sys_net_recv(recv_buffer1, size * PSIZE, size);//, recv_length);
    ret = sys_net_recv(recv_buffer2, size * PSIZE, size);//, recv_length);
    // int ret = do_net_recv((uint64_t)recv_buffer, size * PSIZE, size, (uint64_t)recv_length);
    printf_recv_buffer(recv_buffer1);
    printf_recv_buffer(recv_buffer2);

#endif
    sys_exit();
}
