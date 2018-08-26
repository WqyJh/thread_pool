#ifndef DATA_STRUCTURE_QUEUE_H
#define DATA_STRUCTURE_QUEUE_H

#include <stdbool.h>
#include <stdint.h>


typedef struct queue_s queue_t;
typedef struct qnode_s qnode_t;
typedef union qdata_u qdata_t;


union qdata_u
{
    int8_t i8;
    uint8_t u8;

    int16_t i16;
    uint16_t u16;

    int32_t i32;
    uint32_t u32;

    int64_t i64;
    uint64_t u64;

    void *ptr;
};

struct qnode_s
{
    qnode_t *next;
    qdata_t data;
};

struct queue_s
{
    qnode_t *head;
    qnode_t *tail;
    uint32_t len;
};

/* ---------------- Queue API ---------------- */

bool queue_init(queue_t *queue);

void queue_clear(queue_t *queue);

bool queue_enqueue(queue_t *queue, qdata_t data);

bool queue_dequeue(queue_t *queue, qdata_t *data);

uint32_t queue_len(queue_t *queue);

#define queue_isempty(queue) (queue_len(queue) == 0)

/* ---------------- qnode_t API ---------------- */

qnode_t *qnode_create(qdata_t data);

void qnode_destroy(qnode_t *node);


#endif //DATA_STRUCTURE_QUEUE_H
