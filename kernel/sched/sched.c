#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"
#include "irq.h"

pcb_t pcb[NUM_MAX_TASK];

/* current running task PCB */
pcb_t *current_running;

/* global process id */
pid_t process_id = 1;

/* kernel stack ^_^ */
#define NUM_KERNEL_STACK 0x20

static uint64_t kernel_stack[NUM_KERNEL_STACK];
static int kernel_stack_count = 0;

static uint64_t user_stack[NUM_KERNEL_STACK];
static int user_stack_count = 0;

void init_stack()
{
    uint64_t kernel_stack_top = KERNEL_STACK + KERNEL_STACK_SIZE;
    int i;
    for (i = 0; i < NUM_KERNEL_STACK; i++)
    {
        kernel_stack[i] = kernel_stack_top + i * KERNEL_STACK_SIZE;
    }
    uint64_t user_stack_top = USER_STACK + USER_STACK_SIZE;
    for (i = 0; i < NUM_KERNEL_STACK; i++)
    {
        user_stack[i] = user_stack_top + i * USER_STACK_SIZE;
    }
}

uint64_t new_kernel_stack()
{
    return kernel_stack[kernel_stack_count++];
}

uint64_t new_user_stack()
{
    return user_stack[user_stack_count++];
}

static void free_kernel_stack(uint64_t stack_addr)
{
    current_running->kernel_context.regs[29] = stack_addr;
}

static void free_user_stack(uint64_t stack_addr)
{
    current_running->user_context.regs[29] = stack_addr;
}

/* Process Control Block */
void set_pcb(pid_t pid, pcb_t *pcb, task_info_t *task_info)
{
    // init kernel_context and stack
    pcb->kernel_context.regs[29] = pcb->kernel_stack_top;
    pcb->kernel_context.regs[31] = (uint64_t)exception_handler_exit;
    pcb->kernel_context.cp0_status = initial_cp0_status;
    pcb->kernel_context.cp0_epc = task_info->entry_point;
    // init user_context and stack
    pcb->user_context.regs[29] = pcb->user_stack_top;
    pcb->user_context.regs[31] = task_info->entry_point;
    pcb->user_context.cp0_status = initial_cp0_status;
    pcb->user_context.cp0_epc = task_info->entry_point;
    // init other data
    pcb->base_priority = 1;
    pcb->priority = 1;
    strcpy(pcb->name, task_info->name);
    pcb->pid = pid;
    pcb->which_queue = &ready_queue;
    pcb->type = task_info->type;
    pcb->status = TASK_READY;
    pcb->mode = USER_MODE;
    pcb->cursor_x = 0;
    pcb->cursor_y = 0;
    pcb->sleep_begin_time = 0;
    pcb->sleep_end_time = 0;
    pcb->count = 0;
}

/* ready queue to run */
queue_t ready_queue;

/* block queue to wait */
queue_t block_queue;

static void check_sleeping()
{
    if (queue_is_empty(&block_queue))
    {
        return;
    }
    pcb_t *temp = block_queue.head;
    while (temp != NULL)
    {
        uint32_t current_time = get_timer();
        if ((temp->sleep_end_time < current_time) || (temp->sleep_begin_time > current_time))
        {
            pcb_t *p = queue_remove(&block_queue, temp);
            if (temp->status != TASK_EXITED)
            {
                temp->status = TASK_READY;
            }
            queue_push(&ready_queue, temp);
            temp = p;
        }
        else
        {
            temp = temp->next;
        }
    }
}

void scheduler(void)
{
    if (current_running->mode == KERNEL_MODE)
    {
        // store the cursor
        current_running->cursor_x = screen_cursor_x;
        current_running->cursor_y = screen_cursor_y;

        check_sleeping();
        if (current_running->status != TASK_BLOCKED && current_running->status != TASK_EXITED)
        {
            current_running->status = TASK_READY;
            if (current_running->pid != 0)
            {
                queue_push(&ready_queue, current_running);
            }
        }
        if (!queue_is_empty(&ready_queue))
        {
            // current_running = (pcb_t *)priority_queue_dequeue(&ready_queue);
            current_running = (pcb_t *)queue_dequeue(&ready_queue);
            if (current_running->status == TASK_EXITED)
            {
                do_exit();
            }
        }
        current_running->status = TASK_RUNNING;
        current_running->priority = current_running->base_priority;

        // be schedulered times count ++
        current_running->count += 1;
        // vt100_move_cursor(1, 12);
        // printk("the last do_scheduler() cost time: %d us", current_running->do_scheduler_cost / 150);
        // printk("current_running -> %s", current_running->name);
        // vt100_move_cursor(1, current_running->pid + 12);
        // printk("> %s\t times: %d", current_running->name, current_running->count);
    
        // edit entryhi.asid
        uint64_t entryhi = get_cp0_entryhi();
        entryhi = (entryhi & 0xffffffffffffff00) | current_running->pid;
        set_cp0_entryhi(entryhi);

        // reset the cursor
        screen_cursor_x = current_running->cursor_x;
        screen_cursor_y = current_running->cursor_y;
    }
    else
    {
        // error
        char *input = (char *)0x123455;
        char c = (*input);
    }
}

void do_block(queue_t *queue)
{
    if (current_running->status != TASK_EXITED)
    {
        current_running->status = TASK_BLOCKED;
        queue_push(queue, (void *)current_running);
    }
    do_scheduler();
}

void do_unblock_one(queue_t *queue)
{
    pcb_t *item;
    item = queue_dequeue(queue);
    if (item->status != TASK_EXITED)
    {
        item->status = TASK_READY;
        queue_push(&ready_queue, item);
    }
}

void do_unblock_all(queue_t *queue)
{
    pcb_t *item;
    while (!queue_is_empty(queue))
    {
        item = queue_dequeue(queue);
        if (item->status != TASK_EXITED)
        {
            item->status = TASK_READY;
            queue_push(&ready_queue, item);
        }
    }
}

int do_spawn(task_info_t *task)
{
    int i = 0;
    while (i < NUM_MAX_TASK && pcb[i].status != TASK_EXITED)
    {
        i++;
    }
    set_pcb(process_id, &pcb[i], task);
    // add to ready_queue
    queue_push(&ready_queue, (void *)&pcb[i]);
    process_id++;
    return i;
}

void do_exit(void)
{
    current_running->status = TASK_EXITED;
    // free the wait task queue
    do_unblock_all(&current_running->wait_queue);
    // free stack
    free_kernel_stack(current_running->kernel_stack_top);
    free_user_stack(current_running->user_stack_top);
    // free memory
    bzero(current_running->page_table, sizeof(current_running->page_table));
    // free the lock
    while (!queue_is_empty(&current_running->lock_queue))
    {
        mutex_lock_t *lock = queue_dequeue(&current_running->lock_queue);
        do_unblock_all(&lock->queue);
        lock->status = UNLOCKED;
    }
    do_scheduler();
}

void do_sleep(uint32_t sleep_time)
{
    current_running->status = TASK_BLOCKED;
    current_running->sleep_begin_time = get_timer();
    current_running->sleep_end_time = current_running->sleep_begin_time + sleep_time;
    queue_push(&block_queue, current_running);
    do_scheduler();
}

int do_kill(pid_t pid)
{
    int i = 0;
    if (pid == 0)
    {
        kprintf("kill task0 is not allowed.\n");
        return -1;
    }
    else if (pid == 1)
    {
        kprintf("kill shell_task is not allowed.\n");
        return -1;
    }
    else
    {
        if (current_running->pid == pid)
        {
            do_exit();
            return 0;
        }
        while (pcb[i].pid != pid || pcb[i].status == TASK_EXITED)
        {
            i++;
            if (i >= NUM_MAX_TASK)
            {
                // error
                kprintf("Kill task pid = %d is non-existent.\n", pid);
                return -1;
            }
        }
        pcb[i].status = TASK_EXITED;
        do_unblock_all(&pcb[i].wait_queue);
        // free stack
        free_kernel_stack(pcb[i].kernel_stack_top);
        free_user_stack(pcb[i].user_stack_top);
        // free memory
        bzero(pcb[i].page_table, sizeof(pcb[i].page_table));
        // free the lock
        while (!queue_is_empty(&pcb[i].lock_queue))
        {
            mutex_lock_t *lock = queue_dequeue(&pcb[i].lock_queue);
            do_unblock_all(&lock->queue);
            lock->status = UNLOCKED;
        }
        return 0;
    }
}

int do_waitpid(pid_t pid)
{
    int i = 0;
    while (pcb[i].pid != pid || pcb[i].status == TASK_EXITED)
    {
        i++;
        if (i >= NUM_MAX_TASK)
        {
            // error
            kprintf("\nWait task pid = %d is non-existent.\n", pid);
            return -1;
        }
    }
    if (pcb[i].status != TASK_EXITED)
    {
        current_running->status = TASK_BLOCKED;
        queue_push(&(pcb[i].wait_queue), current_running);
        do_scheduler();
    }
    return 0;
}

// process show
void do_process_show()
{
    kprintf("[PROCESS TABLE]\n");
    int num, items = 0;
    for (num = 0; num < NUM_MAX_TASK; num++)
    {
        switch (pcb[num].status)
        {
        case TASK_BLOCKED:
            kprintf("[%d] PID: %d Name: %s Status : BLOCKED\n", items++, pcb[num].pid, pcb[num].name);
            break;
        case TASK_RUNNING:
            kprintf("[%d] PID: %d Name: %s Status : RUNNING\n", items++, pcb[num].pid, pcb[num].name);
            break;
        case TASK_READY:
            kprintf("[%d] PID: %d Name: %s Status : READY\n", items++, pcb[num].pid, pcb[num].name);
            break;
        default:
            break;
        }
    }
}

pid_t do_getpid()
{
    return current_running->pid;
}

void do_clear()
{
    screen_clear(0, SCREEN_HEIGHT / 2);
    screen_clear(SCREEN_HEIGHT / 2 + 1, SCREEN_HEIGHT);
    screen_move_cursor(0, SCREEN_HEIGHT / 2 + 1);
}