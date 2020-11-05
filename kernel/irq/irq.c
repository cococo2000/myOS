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

extern uint32_t time_elapsed;

static void irq_timer()
{
    screen_reflush();
    /* increase global time counter */
    time_elapsed += 1000000;
    /* reset timer register */
    set_cp0_count(0x00000000);
    set_cp0_compare(TIMER_INTERVAL);
    /* sched.c to do scheduler */
    do_scheduler();
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
