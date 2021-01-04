#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "screen.h"
#include "mac.h"
/* exception handler */
uint64_t exception_handler[32];

/* used to init PCB */
// uint32_t initial_cp0_status = 0x10008003;
// enable mac interrupt
uint32_t initial_cp0_status = 0x10009003;

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

    // check mac wake up?
    // static uint32_t num_package = 0;
    // if (num_package == PNUM) {
    //     num_package = 0;
    // }
    // while (!(0x80000000 & rx_descriptor[num_package % PNUM].tdes0) && num_package < PNUM) {
    //     recv_flag[num_package] = 1;
    //     if (!queue_is_empty(&recv_block_queue)) {
    //         queue_push(&ready_queue, queue_dequeue(&recv_block_queue));
    //     }
    //     num_package++;
    // }

    // uint32_t temp_x;
    // uint32_t temp_y;

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
    // if(count == 1000){
    //     temp_x = screen_cursor_x;
    //     temp_y = screen_cursor_y;
    //     vt100_move_cursor(1, 12);
    //     sum /= 1000;
    //     printk("the average do_scheduler() cost: %d ", sum);
    //     screen_cursor_x = temp_x;
    //     screen_cursor_y = temp_y;
    //     count = 0;
    //     sum = 0;
    // }
    // cost = get_cp0_count();
    do_scheduler();
    // sum += get_cp0_count() - cost;
    // count++;
}

void register_irq_handler(int bit_field, uint64_t irq_handler)
{
    // volatile uint32_t * Inten_0     = (void *)0xffffffffbfe11424;
    volatile uint32_t * Intenset_0  = (void *)0xffffffffbfe11428;
    volatile uint32_t * Intenclr_0  = (void *)0xffffffffbfe1142c;
    volatile uint32_t * Intpol_0    = (void *)0xffffffffbfe11430;
    volatile uint32_t * Intedge_0   = (void *)0xffffffffbfe11434;
    volatile uint32_t * Intbounce_0 = (void *)0xffffffffbfe11438;
    volatile uint32_t * Intauto_0   = (void *)0xffffffffbfe1143c;
    uint32_t set_bit_field_1  = 1 << bit_field;
    uint32_t set_bit_field_0  = ~set_bit_field_1;
    *Intenclr_0  = (*Intenclr_0 ) | set_bit_field_1;
    *Intenset_0  = (*Intenset_0 ) | set_bit_field_1;
    *Intedge_0   = (*Intedge_0  ) | set_bit_field_1;
    *Intauto_0   = (*Intauto_0  ) & set_bit_field_0;
    *Intpol_0    = (*Intpol_0   ) & set_bit_field_0;
    *Intbounce_0 = (*Intbounce_0) & set_bit_field_0;
}

void other_exception_handler()
{
    time_elapsed += 1000000;
    vt100_move_cursor(0, 13);
    printk("Other exception occur: epc = 0x%x, badvaddr = 0x%x", get_cp0_epc(), get_cp0_badvaddr());
    vt100_move_cursor(1, 14);
    printk("cause = 0x%x, status = 0x%x", get_cp0_cause(), get_cp0_status());
}

void interrupt_helper(uint32_t status, uint32_t cause)
{
    current_running->mode = KERNEL_MODE;
    uint32_t interrupt = status & cause & 0x0000ff00; // status_IM[7:0] & cause_IP[7:0]
    if (interrupt & 0x00008000) {
        irq_timer();
    }
    else if (interrupt & 0x00001000) {
        mac_irq_handle();
    }
    else {
        other_exception_handler();
    }
    current_running->mode = USER_MODE;
}
