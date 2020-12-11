#include "sched.h"
#include "screen.h"
#include "stdio.h"
#include "syscall.h"
#include "time.h"

#include "test4.h"

#define RW_TIMES 3

int rand()
{
    int current_time = sys_get_timer();
    return current_time % 100000;
}

void rw_task1(int x, int y, int z)
{
    int mem1, mem2 = 0;
    int curs = 0;
    int memory[RW_TIMES * 2];

    int i = 0;
    int test_mem[RW_TIMES] = {x, y, z};

    // srand((uint32_t)get_ticks());
    for (i = 0; i < RW_TIMES; i++)
    {
        sys_move_cursor(1, curs + i);
        // mem1 = atoi(argv[i + 2]);
        mem1 = test_mem[i];
        memory[i] = mem2 = rand();
        *(int *)mem1 = mem2;
        printf("Write: 0x%x,%d", mem1, mem2);
    }
    curs = RW_TIMES;
    for (i = 0; i < RW_TIMES; i++)
    {
        sys_move_cursor(1, curs + i);
        mem1 = test_mem[i];
        memory[i + RW_TIMES] = *(int *)mem1;
        if (memory[i + RW_TIMES] == memory[i])
            printf("Read succeed: 0x%x,%d", mem1, memory[i + RW_TIMES]);
        else
            printf("Read error: 0x%x,%d", mem1, memory[i + RW_TIMES]);
    }
    sys_exit();
}
