#include "sem.h"
#include "stdio.h"

void do_semaphore_init(semaphore_t *s, int val)
{
    s->semph = val;
    queue_init(&s->semph_queue);
}

void do_semaphore_up(semaphore_t *s)
{
    if (!queue_is_empty(&s->semph_queue))
    {
        do_unblock_one(&s->semph_queue);
    }
    else
    {
        s->semph += 1;
    }
}

void do_semaphore_down(semaphore_t *s)
{
    if (s->semph > 0)
    {
        s->semph -= 1;
    }
    else
    {
        do_block(&s->semph_queue);
    }
}