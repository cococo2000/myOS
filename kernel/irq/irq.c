#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "screen.h"
/* exception handler */
uint64_t exception_handler[32];

/* used to init PCB */
uint32_t initial_cp0_status = 0x10008001;

// extern void do_shell();

// to measure do_scheduler() cost
uint32_t cost = 0;
uint32_t sum = 0;
uint32_t count = 0;

static void irq_timer()
{
    screen_reflush();
    /* increase global time counter */
    time_elapsed += 1000000;
    /* reset timer register */
    reset_timer(TIMER_INTERVAL);

    uint32_t temp_x;
    uint32_t temp_y;

    // if(count > 100 && count % 8){
    //     temp_x = screen_cursor_x;
    //     temp_y = screen_cursor_y;
    //     vt100_move_cursor(1, 13);
    //     printk("current_running -> %s", current_running->name);
    //     vt100_move_cursor(1, current_running->pid + 13);
    //     printk("> %s\t times: %d", current_running->name, current_running->count);
    //     screen_cursor_x = temp_x;
    //     screen_cursor_y = temp_y;
    // }

    /* sched.c to do scheduler */
    if(count == 1000){
        temp_x = screen_cursor_x;
        temp_y = screen_cursor_y;
        vt100_move_cursor(1, 12);
        sum /= 1000;
        printk("the average do_scheduler() cost: %d ", sum);
        screen_cursor_x = temp_x;
        screen_cursor_y = temp_y;
        count = 0;
        sum = 0;
    }
    cost = get_cp0_count();
    do_scheduler();
    sum += get_cp0_count() - cost;
    count++;
}

void other_exception_handler()
{
    time_elapsed += 1000000;
    vt100_move_cursor(1, 28);
    printk("Other exception occur: epc = 0x%x, badvaddr = 0x%x", get_cp0_epc(), get_cp0_badvaddr());
}

void interrupt_helper(uint32_t status, uint32_t cause)
{
    current_running->mode = KERNEL_MODE;
    uint32_t interrupt = status & cause & 0x0000ff00; // status_IM[7:0] & cause_IP[7:0]
    if(interrupt & 0x00008000){
        irq_timer();
    }else{
        other_exception_handler();
    }
    current_running->mode = USER_MODE;
}
