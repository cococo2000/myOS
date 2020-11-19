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
#define NUM_KERNEL_STACK 20

static uint64_t kernel_stack[NUM_KERNEL_STACK];
static int kernel_stack_count;

static uint64_t user_stack[NUM_KERNEL_STACK];
static int user_stack_count;

void init_stack()
{
    
}

uint64_t new_kernel_stack()
{
     
}

uint64_t new_user_stack()
{
    
}

static void free_kernel_stack(uint64_t stack_addr)
{
    
}

static void free_user_stack(uint64_t stack_addr)
{
    
}

/* Process Control Block */
void set_pcb(pid_t pid, pcb_t *pcb, task_info_t *task_info)
{
    
}

/* ready queue to run */
queue_t ready_queue ;

/* block queue to wait */
queue_t block_queue ;

static void check_sleeping()
{
    if(queue_is_empty(&block_queue)){
        return;
    }
    pcb_t* temp = block_queue.head;
    while(temp != NULL){
        uint32_t current_time = get_timer();
        if((temp->sleep_end_time < current_time) || (temp->sleep_begin_time > current_time)){
            pcb_t * p = queue_remove(&block_queue, temp);
            temp->status = TASK_READY;
            queue_push(&ready_queue, temp);
            temp = p;
        }else{
            temp = temp->next;
        }
    }
}

void scheduler(void)
{
    if(current_running->mode == KERNEL_MODE){
        // store the cursor
        current_running->cursor_x = screen_cursor_x;
        current_running->cursor_y = screen_cursor_y;

        check_sleeping();
        if(current_running->status != TASK_BLOCKED && current_running->status != TASK_EXITED){
            current_running->status = TASK_READY;
            if(current_running->pid != 0){
                queue_push(&ready_queue, current_running);
            }
        }
        if(!queue_is_empty(&ready_queue)){
            current_running = (pcb_t *)priority_queue_dequeue(&ready_queue);
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
        // reset the cursor
        screen_cursor_x = current_running->cursor_x;
        screen_cursor_y = current_running->cursor_y;
    }else{
        // error
        char * input = (char *)0x123455;
        char c = (*input);
    }
}

void do_block(queue_t *queue)
{
    current_running->status = TASK_BLOCKED;
    queue_push(queue, (void*)current_running);
}

void do_unblock_one(queue_t *queue)
{
    pcb_t *item;
    item = queue_dequeue(queue);
    item->status = TASK_READY;
    queue_push(&ready_queue, item);
}

void do_unblock_all(queue_t *queue)
{
    pcb_t *item;
    while(!queue_is_empty(queue)){
        item = queue_dequeue(queue);
        item->status = TASK_READY;
        queue_push(&ready_queue, item);
    }
}

int do_spawn(task_info_t *task)
{
    //  init pcb all 0
    bzero(&pcb[process_id], sizeof(pcb_t));
    // init kernel_context and stack
    pcb[process_id].kernel_stack_top = stack_top;
    pcb[process_id].kernel_context.regs[29] = stack_top;
    pcb[process_id].kernel_context.regs[31] = (uint64_t)exception_handler_exit;
    pcb[process_id].kernel_context.cp0_status = initial_cp0_status;
    pcb[process_id].kernel_context.cp0_epc = task->entry_point;
    stack_top -= PCB_STACK_SIZE;
    // init user_context and stack
    pcb[process_id].user_stack_top = stack_top;
    pcb[process_id].user_context.regs[29] = stack_top;
    pcb[process_id].user_context.regs[31] = task->entry_point;
    pcb[process_id].user_context.cp0_status = initial_cp0_status;
    pcb[process_id].user_context.cp0_epc = task->entry_point;
    stack_top -= PCB_STACK_SIZE;
    // init other data
    pcb[process_id].base_priority = 1;
    pcb[process_id].priority = 1;
    strcpy(pcb[process_id].name, task->name);
    pcb[process_id].pid = process_id;
    pcb[process_id].which_queue = &ready_queue;
    pcb[process_id].type = task->type;
    pcb[process_id].status = TASK_READY;
    pcb[process_id].mode = USER_MODE;
    pcb[process_id].cursor_x = 0;
    pcb[process_id].cursor_y = 0;
    pcb[process_id].sleep_begin_time = 0;
    pcb[process_id].sleep_end_time = 0;
    pcb[process_id].count = 0;
    // add to ready_queue
    queue_push(&ready_queue, (void *)&pcb[process_id]);
    process_id++;
}

void do_exit(void)
{
    current_running->status = TASK_EXITED;
    do_unblock_all(&current_running->wait_queue);
    int i;
    while (!queue_is_empty(&current_running->lock_queue)) {
        mutex_lock_t * lock = queue_dequeue(&current_running->lock_queue);
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
    
}

int do_waitpid(pid_t pid)
{
    
}

// process show
void do_process_show()
{
    kprintf("[PROCESS TABLE]\n");
    int num, items = 0;
    for (num = 0; num < NUM_MAX_TASK; num++) {
        switch(pcb[num].status) {
            case TASK_BLOCKED:
                kprintf("[%d] PID : %d  Status : BLOCKED\n", items++, pcb[num].pid);
                break;
            case TASK_RUNNING:
                kprintf("[%d] PID : %d  Status : RUNNING\n", items++, pcb[num].pid);
                break;
            case TASK_READY:
                kprintf("[%d] PID : %d  Status : READY\n", items++, pcb[num].pid);
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
    screen_clear(0, SCREEN_HEIGHT / 2 - 1);
    screen_move_cursor(0, SCREEN_HEIGHT / 2 + 1);
}