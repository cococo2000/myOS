/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include "fs.h"
#include "irq.h"
#include "test.h"
#include "stdio.h"
#include "sched.h"
#include "screen.h"
#include "common.h"
#include "syscall.h"
#include "smp.h"
#include "mm.h"
#include "mac.h"

#define TASK_INIT (00)
#define STACK_MAX 0xffffffffa0f00000
#define STACK_MIN 0xffffffffa0d00000
#define PCB_STACK_SIZE 0x10000
uint64_t stack_top = STACK_MAX;
extern struct task_info *sched1_tasks[16];
extern int num_sched1_tasks;
extern struct task_info *lock_tasks[16];
extern int num_lock_tasks;
extern struct task_info *timer_tasks[16];
extern int num_timer_tasks;
extern struct task_info *sched2_tasks[16];
extern int num_sched2_tasks;
static void init_memory()
{
}
static void init_pcb()
{
    queue_init(&ready_queue);
    queue_init(&block_queue);
    // queue_init(&sleep_queue);
    int num_total_tasks_groups = 3;
    int each_tasks_num[4] = {num_lock_tasks, num_timer_tasks, num_sched2_tasks};
    struct task_info ** p_task_info[4] = {lock_tasks, timer_tasks, sched2_tasks};
    int cur_queue_id = 0;
    // pcb[0] init
    bzero(&pcb[cur_queue_id], sizeof(pcb_t));
    pcb[cur_queue_id].kernel_stack_top = stack_top;
    pcb[cur_queue_id].kernel_context.regs[29] = stack_top;
    stack_top -= PCB_STACK_SIZE;
    pcb[cur_queue_id].kernel_context.cp0_status = initial_cp0_status;
    pcb[cur_queue_id].user_stack_top = stack_top;
    pcb[cur_queue_id].user_context.regs[29] = stack_top;
    stack_top -= PCB_STACK_SIZE;
    pcb[cur_queue_id].user_context.cp0_status = initial_cp0_status;
    pcb[cur_queue_id].priority = 1;
    strcpy(pcb[cur_queue_id].name, "task0 ");
    pcb[cur_queue_id].pid = process_id++;
    pcb[cur_queue_id].status = TASK_RUNNING;
    pcb[cur_queue_id].cursor_x = 0;
    pcb[cur_queue_id].cursor_y = 0;
    pcb[cur_queue_id].sleep_begin_time = 0;
    pcb[cur_queue_id].sleep_end_time = 0;
    pcb[cur_queue_id].count = 0;
    cur_queue_id++;
    //scheduler1 task1
    int i, j;
    for(j = 0; j < num_total_tasks_groups; j++){
        for(i = 0; i < each_tasks_num[j]; i++, cur_queue_id++)
        {
            // init pcb all 0
            bzero(&pcb[cur_queue_id], sizeof(pcb_t));
            // init kernel_context and stack
            pcb[cur_queue_id].kernel_stack_top = stack_top;
            pcb[cur_queue_id].kernel_context.regs[29] = stack_top;
            stack_top -= PCB_STACK_SIZE;
            pcb[cur_queue_id].kernel_context.regs[31] = (uint64_t)exception_handler_exit;
            pcb[cur_queue_id].kernel_context.cp0_status = initial_cp0_status;
            pcb[cur_queue_id].kernel_context.cp0_epc = p_task_info[j][i]->entry_point;
            // init user_context and stack
            pcb[cur_queue_id].user_stack_top = stack_top;
            pcb[cur_queue_id].user_context.regs[29] = stack_top;
            stack_top -= PCB_STACK_SIZE;
            pcb[cur_queue_id].user_context.regs[31] = p_task_info[j][i]->entry_point;
            pcb[cur_queue_id].user_context.cp0_status = initial_cp0_status;
            pcb[cur_queue_id].user_context.cp0_epc = p_task_info[j][i]->entry_point;
            // init other data
            pcb[cur_queue_id].prev = NULL;
            pcb[cur_queue_id].next = NULL;
            pcb[cur_queue_id].priority = 1;
            strcpy(pcb[cur_queue_id].name, p_task_info[j][i]->name);
            pcb[cur_queue_id].pid = process_id++;
            pcb[cur_queue_id].type = p_task_info[j][i]->type;
            pcb[cur_queue_id].status = TASK_READY;
            pcb[cur_queue_id].cursor_x = 0;
            pcb[cur_queue_id].cursor_y = 0;
            pcb[cur_queue_id].sleep_begin_time = 0;
            pcb[cur_queue_id].sleep_end_time = 0;
            pcb[cur_queue_id].count = 0;
            // add to ready_queue
            queue_push(&ready_queue, (void *)&pcb[cur_queue_id]);
        }
    }
    // init current_running pointer to pcb[0]
    current_running = &pcb[0];
}

static void init_exception_handler()
{
    int i;
    for(i = 1; i < 32; i++){
        exception_handler[i]=(uint64_t)handle_other;
    }
    exception_handler[INT]=(uint64_t)handle_int;
    exception_handler[SYS]=(uint64_t)handle_syscall;
}

static void init_exception()
{
    /* fill nop */
    init_exception_handler();
    /* fill nop */
    memcpy(0xffffffff80000180, exception_handler_entry, (char *)exception_handler_end - (char *)exception_handler_begin);
    set_cp0_cause(0x00000000);
    /* set COUNT & set COMPARE */
    /* open interrupt */
    set_cp0_count(0x00000000);
    set_cp0_compare(TIMER_INTERVAL);
}

// [2]
// extern int read_shell_buff(char *buff);

static void init_syscall(void)
{
    syscall[SYSCALL_SLEEP] = (uint64_t (*)())do_sleep;
    syscall[SYSCALL_WRITE] = (uint64_t (*)())screen_write;
    syscall[SYSCALL_CURSOR] = (uint64_t (*)())screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = (uint64_t (*)())screen_reflush;
    syscall[SYSCALL_MUTEX_LOCK_INIT] = (uint64_t (*)())do_mutex_lock_init;
    syscall[SYSCALL_MUTEX_LOCK_ACQUIRE] = (uint64_t (*)())do_mutex_lock_acquire;
    syscall[SYSCALL_MUTEX_LOCK_RELEASE] = (uint64_t (*)())do_mutex_lock_release;
}

/* [0] The beginning of everything >_< */
void __attribute__((section(".entry_function"))) _start(void)
{
    asm_start();

    /* init stack space */
    init_stack();
    printk("> [INIT] Stack heap initialization succeeded.\n");

    /* init interrupt */
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n");

    init_memory();
    printk("> [INIT] Virtual memory initialization succeeded.\n");
    // init system call table (0_0)
    /* init system call table */

    init_syscall();
    printk("> [INIT] System call initialized successfully.\n");

    /* init Process Control Block */

    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n");

    /* init screen */
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n");

    /* init filesystem */
    // read_super_block();

    /* wake up core1*/
    // loongson3_boot_secondary();

    /* set cp0_status register to allow interrupt */
    // enable exception and interrupt
    // ERL = 0, EXL = 0, IE = 1
    set_cp0_status(initial_cp0_status);

    while (1)
    {
        // current_running->status = TASK_EXITED;
        do_scheduler();
    };
    return;
}
