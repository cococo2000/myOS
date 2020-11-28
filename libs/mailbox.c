#include "string.h"
#include "mailbox.h"

#define MAX_NUM_BOX 32

mailbox_t mboxs[MAX_NUM_BOX];
mutex_lock_t alloc_lock;

static int mbox_is_full(mailbox_t *mailbox)
{
}

static int mbox_is_empty(mailbox_t *mailbox)
{
}

static void init_mbox(mailbox_t *mailbox)
{
}

void mbox_init()
{
    int i;
    mutex_lock_init(&alloc_lock);
    for (i = 0; i < MAX_NUM_BOX; i++)
    {
        mboxs[i].name[0] = '\0';
        mboxs[i].msg_head = mboxs[i].msg_tail = 0;
        mboxs[i].used_size = 0;
        mboxs[i].cited = 0;
        condition_init(&mboxs[i].full);
        condition_init(&mboxs[i].empty);
        mutex_lock_init(&mboxs[i].mutex);
    }
}

mailbox_t *mbox_open(char *name)
{
    int i = 0;
    mutex_lock_acquire(&alloc_lock);
    for (i = 0; i < MAX_NUM_BOX; i++)
    {
        if (!strcmp(name, mboxs[i].name))
        {
            mboxs[i].cited++;
            mutex_lock_release(&alloc_lock);
            return &mboxs[i];
        }
    }
    for (i = 0; i < MAX_NUM_BOX; i++)
    {
        if (mboxs[i].name[0] == '\0')
        {
            strcpy(mboxs[i].name, name);
            mboxs[i].cited++;
            mutex_lock_release(&alloc_lock);
            return &mboxs[i];
        }
    }
    mutex_lock_release(&alloc_lock);
}

void mbox_close(mailbox_t *mailbox)
{
    mutex_lock_acquire(&mailbox->mutex);
    if (mailbox->cited == 0)
    {
        mailbox->name[0] = '\0';
        mailbox->msg_head = mailbox->msg_tail = 0;
        mailbox->used_size = 0;
        mailbox->cited = 0;
        condition_init(&mailbox->full);
        condition_init(&mailbox->empty);
        mutex_lock_init(&mailbox->mutex);
    }
    mutex_lock_release(&mailbox->mutex);
}

void mbox_send(mailbox_t *mailbox, void *msg, int msg_length)
{
    mutex_lock_acquire(&mailbox->mutex);
    while (mailbox->used_size + msg_length > MSG_MAX_SIZE)
    // mailbox is full
    {
        condition_wait(&mailbox->mutex, &mailbox->full);
    }
    if (mailbox->msg_tail + msg_length > MSG_MAX_SIZE)
    {
        memcpy((char *)(mailbox->msg + mailbox->msg_tail), (char *)msg, MSG_MAX_SIZE - mailbox->msg_tail);
        mailbox->msg_tail = msg_length - (MSG_MAX_SIZE - mailbox->msg_tail);
        memcpy((char *)mailbox->msg, ((char *)msg + msg_length - mailbox->msg_tail), mailbox->msg_tail);
    }
    else
    {
        memcpy((char *)(mailbox->msg + mailbox->msg_tail), (char *)msg, msg_length);
        mailbox->msg_tail += msg_length;
    }
    mailbox->used_size += msg_length;
    condition_broadcast(&mailbox->empty);
    mutex_lock_release(&mailbox->mutex);
}

void mbox_recv(mailbox_t *mailbox, void *msg, int msg_length)
{
    mutex_lock_acquire(&mailbox->mutex);
    while (mailbox->used_size < msg_length)
    {
        condition_wait(&mailbox->mutex, &mailbox->empty);
    }
    if (mailbox->msg_tail + msg_length > MSG_MAX_SIZE)
    {
        memcpy((char *)msg, (char *)(mailbox->msg + mailbox->msg_head), MSG_MAX_SIZE - mailbox->msg_head);
        mailbox->msg_head = msg_length - (MSG_MAX_SIZE - mailbox->msg_head);
        memcpy(((char *)msg + msg_length - mailbox->msg_head), (char *)mailbox->msg, mailbox->msg_head);
    }
    else
    {
        memcpy((char *)msg, (char *)(mailbox->msg + mailbox->msg_head), msg_length);
        mailbox->msg_head += msg_length;
    }
    mailbox->used_size -= msg_length;
    condition_broadcast(&mailbox->full);
    mutex_lock_release(&mailbox->mutex);
}