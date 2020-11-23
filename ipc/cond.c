#include "cond.h"
#include "lock.h"

/* clear queue */
void do_condition_init(condition_t *condition)
{
    queue_init(&condition->cond_queue);
}

/* The lock has been obtained before the task calls the function */
void do_condition_wait(mutex_lock_t *lock, condition_t *condition)
{
    /* [1] release the lock so other task can operate on the condition */
    do_mutex_lock_release(lock);

    /* [2] do scheduler */
    do_block(&condition->cond_queue);

    /* [3] acquire lock */
    do_mutex_lock_acquire(lock);
}

/* unblock one task */
void do_condition_signal(condition_t *condition)
{
    if (!queue_is_empty(&condition->cond_queue)){
        do_unblock_one(&condition->cond_queue);
    }
}

/* unblock all task */
void do_condition_broadcast(condition_t *condition)
{
    do_unblock_all(&condition->cond_queue);
}