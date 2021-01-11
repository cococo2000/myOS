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
#include "smp.h"
#include "mm.h"
#include "mac.h"
#include "time.h"
#include "fs.h"
#include "syscall.h"

#define TASK_INIT (00)

static void init_memory()
{
    set_cp0_wired(32);
    // task 1
    // init_TLB();

    // task 2
    // init_page_table();
}

static void init_pcb0(){
    pcb[0].kernel_context.regs[29] = pcb[0].kernel_stack_top;
    pcb[0].kernel_context.regs[31] = (uint64_t)exception_handler_exit;
    pcb[0].kernel_context.cp0_status = initial_cp0_status;
    pcb[0].user_context.regs[29] = pcb[0].user_stack_top;
    pcb[0].user_context.cp0_status = initial_cp0_status;
    pcb[0].base_priority = 1;
    pcb[0].priority = 1;
    strcpy(pcb[0].name, "task0");
    pcb[0].pid = 0;
    pcb[0].which_queue = &ready_queue;
    queue_init(&pcb[0].wait_queue);
    queue_init(&pcb[0].lock_queue);
    pcb[0].type = KERNEL_PROCESS;
    pcb[0].status = TASK_RUNNING;
    pcb[0].mode = KERNEL_MODE;
    pcb[0].cursor_x = 0;
    pcb[0].cursor_y = 0;
    pcb[0].sleep_begin_time = 0;
    pcb[0].sleep_end_time = 0;
    pcb[0].count = 0;
}

static void init_pcb()
{
    int i;
    for (i = 0; i < NUM_MAX_TASK; i++) {
        bzero(&pcb[i], sizeof(pcb_t));
        pcb[i].status = TASK_EXITED;
        pcb[i].kernel_stack_top = new_kernel_stack();
        pcb[i].user_stack_top = new_user_stack();
    }
    queue_init(&ready_queue);
    queue_init(&block_queue);
    // pcb[0] init, pcb = 0
    init_pcb0();
    // shell init
    int shell_pid = 1;  // = (init) process_id
    do_spawn(&shell_task);
    queue_push(&ready_queue, (void *)&pcb[shell_pid]);
    // init current_running pointer to pcb[0]
    current_running = &pcb[0];
}

static void init_exception_handler()
{
    int i;
    for(i = 1; i < 32; i++){
        exception_handler[i] = (uint64_t)handle_other;
    }
    exception_handler[INT ] = (uint64_t)handle_int;
    // exception_handler[MOD ] = (uint64_t)handle_mod;
    exception_handler[TLBL] = (uint64_t)handle_tlb;
    exception_handler[TLBS] = (uint64_t)handle_tlb;
    exception_handler[SYS ] = (uint64_t)handle_syscall;
}

static void init_exception()
{
    /* fill nop */
    init_exception_handler();
    /* fill nop */
    // exception_handler_entry
    memcpy(0xffffffff80000180, exception_handler_entry, (char *)exception_handler_end - (char *)exception_handler_begin);
    // TLB exception_handler_entry
    memcpy(0xffffffff80000000, TLBexception_handler_entry, (char *)TLBexception_handler_end - (char *)TLBexception_handler_begin);
    set_cp0_cause(0x00000000);
    // set_cp0_status(initial_cp0_status);
    /* set COUNT & set COMPARE */
    /* open interrupt */
    set_cp0_count(0x00000000);
    set_cp0_compare(TIMER_INTERVAL);
}

// [2]
// extern int read_shell_buff(char *buff);
extern char read_shell_buff();

static void init_syscall(void)
{
    // init system call table
    syscall[SYSCALL_SPAWN              ] = (uint64_t (*)())do_spawn;
    syscall[SYSCALL_EXIT               ] = (uint64_t (*)())do_exit;
    syscall[SYSCALL_SLEEP              ] = (uint64_t (*)())do_sleep;
    syscall[SYSCALL_KILL               ] = (uint64_t (*)())do_kill;
    syscall[SYSCALL_WAITPID            ] = (uint64_t (*)())do_waitpid;
    syscall[SYSCALL_PS                 ] = (uint64_t (*)())do_process_show;
    syscall[SYSCALL_GETPID             ] = (uint64_t (*)())do_getpid;
    
    syscall[SYSCALL_GET_TIMER          ] = (uint64_t (*)())get_timer;
    syscall[SYSCALL_YIELD              ] = (uint64_t (*)())do_scheduler;

    syscall[SYSCALL_WRITE              ] = (uint64_t (*)())screen_write;
    // syscall[SYSCALL_READ               ] = (uint64_t (*)())NULL;
    syscall[SYSCALL_CURSOR             ] = (uint64_t (*)())screen_move_cursor;
    syscall[SYSCALL_REFLUSH            ] = (uint64_t (*)())screen_reflush;
    // syscall[SYSCALL_SERIAL_READ        ] = (uint64_t (*)())NULL;
    // syscall[SYSCALL_SERIAL_WRITE       ] = (uint64_t (*)())NULL;
    syscall[SYSCALL_READ_SHELL_BUFF    ] = (uint64_t (*)())read_shell_buff;
    syscall[SYSCALL_SCREEN_CLEAR       ] = (uint64_t (*)())do_clear;

    syscall[SYSCALL_MUTEX_LOCK_INIT    ] = (uint64_t (*)())do_mutex_lock_init;
    syscall[SYSCALL_MUTEX_LOCK_ACQUIRE ] = (uint64_t (*)())do_mutex_lock_acquire;
    syscall[SYSCALL_MUTEX_LOCK_RELEASE ] = (uint64_t (*)())do_mutex_lock_release;

    syscall[SYSCALL_CONDITION_INIT     ] = (uint64_t (*)())do_condition_init;
    syscall[SYSCALL_CONDITION_WAIT     ] = (uint64_t (*)())do_condition_wait;
    syscall[SYSCALL_CONDITION_SIGNAL   ] = (uint64_t (*)())do_condition_signal;
    syscall[SYSCALL_CONDITION_BROADCAST] = (uint64_t (*)())do_condition_broadcast;

    syscall[SYSCALL_SEMAPHORE_INIT     ] = (uint64_t (*)())do_semaphore_init;
    syscall[SYSCALL_SEMAPHORE_UP       ] = (uint64_t (*)())do_semaphore_up;
    syscall[SYSCALL_SEMAPHORE_DOWN     ] = (uint64_t (*)())do_semaphore_down;

    syscall[SYSCALL_BARRIER_INIT       ] = (uint64_t (*)())do_barrier_init;
    syscall[SYSCALL_BARRIER_WAIT       ] = (uint64_t (*)())do_barrier_wait;

    syscall[SYSCALL_WAIT_RECV_PACKAGE  ] = (uint64_t (*)())do_wait_recv_package;
    syscall[SYSCALL_NET_RECV           ] = (uint64_t (*)())do_net_recv;
    syscall[SYSCALL_NET_SEND           ] = (uint64_t (*)())do_net_send;
    syscall[SYSCALL_INIT_MAC           ] = (uint64_t (*)())do_init_mac;
}

/* [0] The beginning of everything >_< */
void __attribute__((section(".entry_function"))) _start(void)
{
    // asm_start();

    /* init Process Control Block */
    /* init stack space */
    init_stack();
    init_pcb();
    printk("> [INIT] Stack heap initialization succeeded.\n\r");
    printk("> [INIT] PCB initialization succeeded.\n\r");

    /* init interrupt */
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

    init_memory();
    printk("> [INIT] Virtual memory initialization succeeded.\n\r");

    // init system call table (0_0)
    /* init system call table */
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n\r");

    do_init_mac();

    /* init screen */
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n\r");
    
    /* init file system */
    init_fs();
    printk("> [INIT] FS initialization succeeded.\n\r");

    /* init filesystem */
    // read_super_block();

    /* wake up core1*/
    // loongson3_boot_secondary();

    /* set cp0_status register to allow interrupt */
    // enable exception and interrupt
    // ERL = 0, EXL = 0, IE = 1
    set_cp0_status(0x10009001);

    while (1)
    {
        do_scheduler();
    };
    return;
}
