#include "queue.h"
#include "sched.h"

typedef pcb_t item_t;

void queue_init(queue_t *queue)
{
    // if (queue->head = queue->tail = NULL) the queue is empty
    queue->head = queue->tail = NULL;
}

int queue_is_empty(queue_t *queue)
{
    if (queue->head == NULL)
    {
        return 1;
    }
    return 0;
}

void queue_push(queue_t *queue, void *item)
{
     item_t *_item= (item_t *)item;
    
    // queue is empty
    if (queue->head == NULL)
    {
        queue->head = item;
        queue->tail = item;
        _item->next = NULL;
        _item->prev = NULL;
    }
    // insert into the last queue
    else
    {
        ((item_t *)(queue->tail))->next = item;
        _item->next = NULL;
        _item->prev = queue->tail;
        queue->tail = item;
    }
}

void *queue_dequeue(queue_t *queue)
{
    item_t *temp = (item_t *)queue->head;

    // this queue only has one item
    if (temp->next == NULL)
    {
        queue->head = NULL;
        queue->tail = NULL;
    }
    else
    {
        queue->head = ((item_t *)(queue->head))->next;
        ((item_t *)(queue->head))->prev = NULL;
    }

    // head
    temp->prev = NULL;
    temp->next = NULL;

    return (void *)temp;
}

/* remove this item and return next item */
void *queue_remove(queue_t *queue, void *item)
{
    item_t *_item = (item_t *)item;
    item_t *next = (item_t *)_item->next;

    // only one item
    if (item == queue->head && item == queue->tail)
    {
        queue->head = NULL;
        queue->tail = NULL;
    }
    // remove the first item
    else if (item == queue->head)
    {
        queue->head = _item->next;
        ((item_t *)(queue->head))->prev = NULL;
    }
    // remove the last item
    else if (item == queue->tail)
    {
        queue->tail = _item->prev;
        ((item_t *)(queue->tail))->next = NULL;
    }
    // remove middle item
    else
    {
        ((item_t *)(_item->prev))->next = _item->next;
        ((item_t *)(_item->next))->prev = _item->prev;
    }

    _item->prev = NULL;
    _item->next = NULL;

    return (void *)next;
}

void *priority_queue_dequeue(queue_t *queue)
{
    item_t *temp = (item_t *)queue->head;
    item_t *max_priority = temp;
    uint32_t max_priority_value = temp->priority;
    while(temp != NULL){
        temp->priority += 1;
        if(temp->priority > max_priority_value){
            max_priority = temp;
            max_priority_value = temp->priority;
        }
        temp = temp->next;
    }
    // head
    queue_remove(queue, max_priority);
    return (void *)max_priority;
}