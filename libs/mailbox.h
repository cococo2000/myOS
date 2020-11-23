#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#include "type.h"
#include "sync.h"

#define MAX_MBOX_LENGTH (64)
#define MSG_MAX_SIZE 100
typedef struct mailbox
{
    char name[30];
    uint8_t msg[MSG_MAX_SIZE];
    int msg_head, msg_tail;
    int used_size;
    int cited;
    condition_t full;
    condition_t empty;
    mutex_lock_t mutex;
} mailbox_t;

void mbox_init();
mailbox_t *mbox_open(char *);
void mbox_close(mailbox_t *);
void mbox_send(mailbox_t *, void *, int);
void mbox_recv(mailbox_t *, void *, int);

#endif