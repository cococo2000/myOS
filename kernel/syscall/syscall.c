#include "fs.h"
#include "sem.h"
#include "irq.h"
#include "cond.h"
#include "lock.h"
#include "sched.h"
#include "common.h"
#include "screen.h"
#include "barrier.h"
#include "syscall.h"

void system_call_helper(uint64_t fn, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    current_running->mode = KERNEL_MODE;
    uint64_t ret_val = syscall[fn] (arg1,arg2,arg3);
    current_running->user_context.regs[2] = ret_val;
    current_running->mode = USER_MODE;
}

void sys_spawn(task_info_t *info)
{
    invoke_syscall(SYSCALL_SPAWN, (uint64_t)info, IGNORE, IGNORE);
}

void sys_exit(void)
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE);
}

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, (uint64_t)time, IGNORE, IGNORE);
}

int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, (uint64_t)pid, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    return invoke_syscall(SYSCALL_WAITPID, (uint64_t)pid, IGNORE, IGNORE);
}

void sys_process_show(void)
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid()
{
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE);
}


uint32_t sys_get_timer(){
    return invoke_syscall(SYSCALL_GET_TIMER, IGNORE, IGNORE, IGNORE);
}

void sys_yield(){
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
}


void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (uint64_t)buff, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, (uint64_t)x, (uint64_t)y, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(int line1, int line2)
{
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE);
}


void mutex_lock_init(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_INIT, (uint64_t)lock, IGNORE, IGNORE);
}

void mutex_lock_acquire(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_ACQUIRE, (uint64_t)lock, IGNORE, IGNORE);
}

void mutex_lock_release(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_RELEASE, (uint64_t)lock, IGNORE, IGNORE);
}


void condition_init(condition_t *condition)
{
    invoke_syscall(SYSCALL_CONDITION_INIT, (uint64_t)condition, IGNORE, IGNORE);
}

void condition_wait(mutex_lock_t *lock, condition_t *condition)
{
    invoke_syscall(SYSCALL_CONDITION_WAIT, (uint64_t)lock, (uint64_t)condition, IGNORE);
}

void condition_signal(condition_t *condition)
{
    invoke_syscall(SYSCALL_CONDITION_SIGNAL, (uint64_t)condition, IGNORE, IGNORE);
}

void condition_broadcast(condition_t *condition)
{
    invoke_syscall(SYSCALL_CONDITION_BROADCAST, (uint64_t)condition, IGNORE, IGNORE);
}


void semaphore_init(semaphore_t *s, int val)
{
    invoke_syscall(SYSCALL_SEMAPHORE_INIT, (uint64_t)s, (uint64_t)val, IGNORE);
}
void semaphore_up(semaphore_t *s)
{
    invoke_syscall(SYSCALL_SEMAPHORE_UP, (uint64_t)s, IGNORE, IGNORE);
}

void semaphore_down(semaphore_t *s)
{
    invoke_syscall(SYSCALL_SEMAPHORE_DOWN, (uint64_t)s, IGNORE, IGNORE);
}


void barrier_init(barrier_t *barrier, int goal)
{
    invoke_syscall(SYSCALL_BARRIER_INIT, (uint64_t)barrier, (uint64_t)goal, IGNORE);
}

void barrier_wait(barrier_t *barrier)
{
    invoke_syscall(SYSCALL_BARRIER_WAIT, (uint64_t)barrier, IGNORE, IGNORE);
}


// int sys_read_shell_buff(char *buff)
// {
// }
char sys_read_shell_buff()
{
    return invoke_syscall(SYSCALL_READ_SHELL_BUFF, IGNORE, IGNORE, IGNORE);
}

void sys_mkfs()
{
}

int sys_mkdir(char *name)
{
}

int sys_readdir(char *name)
{
}

int sys_enterdir(char *name)
{
}

int sys_rmdir(char *name)
{
}

int sys_print_fs(char *name)
{
}

int sys_mknod(char *name)
{
}

int sys_fopen(char *name, uint32_t access)
{
}

int sys_fwrite(uint32_t fd, char *buff, uint32_t size)
{
}

int sys_cat(char *name)
{
}

int sys_fread(uint32_t fd, char *buff, uint32_t size)
{
}

int sys_close(uint32_t fd)
{
}

uint32_t sys_net_recv(uint64_t rd, uint64_t rd_phy, uint64_t daddr)
{
}

void sys_net_send(uint64_t td, uint64_t td_phy)
{
}

void sys_init_mac()
{
}