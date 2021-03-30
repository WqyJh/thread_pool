#include "queue.h"

#include <stddef.h>
#include <strings.h>
#include <stdlib.h>


/* ---------------- Queue API ---------------- */


bool queue_init(queue_t *queue)
{
    bool status = false;

    if (queue == NULL) {
        goto EXIT;
    }

    bzero(queue, sizeof(queue_t));
    status = true;

EXIT:
    return status;
}


void queue_clear(queue_t *queue)
{
    qdata_t data;

    if (queue) {
        while (queue_dequeue(queue, &data)) {};
    }
}


bool queue_enqueue(queue_t *queue, qdata_t data)
{
    bool status = false;
    qnode_t *node = NULL;

    if (queue == NULL) {
        goto EXIT;
    }

    node = qnode_create(data);

    if (node == NULL) {
        goto EXIT;
    }

    if (queue->len == 0) {
        queue->head = node;
    } else if (queue->len == 1) {
        queue->head->next = node;
    } else {
        queue->tail->next = node;
    }

    queue->tail = node;
    ++queue->len;
    status = true;

EXIT:
    return status;
}


bool queue_dequeue(queue_t *queue, qdata_t *data)
{
    bool status = false;
    qnode_t *node = NULL;

    if (queue == NULL || queue->len == 0 || data == NULL) {
        goto EXIT;
    }

    node = queue->head;
    *data = node->data;

    if (queue->len == 1) {
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->head = queue->head->next;
    }

    qnode_destroy(node);
    --queue->len;
    status = true;

EXIT:
    return status;
}


uint32_t queue_len(queue_t *queue)
{
    uint32_t len = 0;

    if (queue) {
        len = queue->len;
    }

    return len;
}


/* ---------------- qnode_t API ---------------- */


qnode_t *qnode_create(qdata_t data)
{
    qnode_t *node = NULL;

    node = calloc(1, sizeof(qnode_t));

    if (node) {
        node->data = data;
    }

    return node;
}


void qnode_destroy(qnode_t *node)
{
    if (node) {
        free(node);
    }
}
