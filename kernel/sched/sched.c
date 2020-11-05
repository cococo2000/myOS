#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"

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
    uint32_t current_time = get_timer();
    if(queue_is_empty(&block_queue)){
        return;
    }
    pcb_t* temp = block_queue.head;
    while(temp != NULL){
        if((temp->sleep_end_time < current_time) || (temp->sleep_begin_time > current_time)){
            queue_remove(&block_queue, temp);
            temp->status = TASK_READY;
            queue_push(&ready_queue, temp);
        }
        temp = temp->next;
    }
}

void scheduler(void)
{
    current_running->cursor_x = screen_cursor_x;
    current_running->cursor_y = screen_cursor_y;

    check_sleeping();
    if(current_running->status != TASK_BLOCKED){
        current_running->status = TASK_READY;
        if(current_running->pid != 1){
            queue_push(&ready_queue, current_running);
        }
    }
    if(!queue_is_empty(&ready_queue)){
        current_running = (pcb_t *)queue_dequeue(&ready_queue);
    }
    current_running->status = TASK_RUNNING;

    current_running->count += 1;
    vt100_move_cursor(1, 11);
    printk("current_running -> %s", current_running->name);
    vt100_move_cursor(1, current_running->pid + 10);
    printk("%s\t times: %d", current_running->name, current_running->count);
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
}

void do_sleep(uint32_t sleep_time)
{
    current_running->status = TASK_BLOCKED;
    current_running->sleep_begin_time = get_timer();
    current_running->sleep_end_time = current_running->sleep_begin_time + sleep_time;
    queue_push(&block_queue, current_running);
    do_scheduler();
}

void do_exit(void)
{
    
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

     
}

pid_t do_getpid()
{
     
}