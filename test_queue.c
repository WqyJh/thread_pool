#include "queue.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define LEN 5
int data[LEN] = {1, 2, 5, 4, 3};


void test_init()
{
    fprintf(stderr, "test_init() started\n");

    queue_t queue;

    queue_init(&queue);

    assert(0 == queue.len);
    assert(NULL == queue.head);
    assert(NULL == queue.tail);

    fprintf(stderr, "test_init() succeed\n");
}


void test_destroy()
{
    fprintf(stderr, "test_destroy() started\n");

    qdata_t qdata;
    queue_t queue;

    assert(queue_init(&queue));

    for (int i = 0; i < LEN; ++i) {
        qdata.i32 = data[i];
        assert(queue_enqueue(&queue, qdata));
    }

    assert(LEN == queue_len(&queue));

    queue_clear(&queue);
    assert(0 == queue.len);
    assert(NULL == queue.head);
    assert(NULL == queue.tail);

    fprintf(stderr, "test_destroy() succeed\n");
}


void test_len()
{
    fprintf(stderr, "test_len() started\n");

    qdata_t qdata;
    queue_t queue;

    assert(queue_init(&queue));

    for (int i = 0; i < LEN; ++i) {
        qdata.i32 = data[i];
        assert(queue_enqueue(&queue, qdata));
    }

    assert(LEN == queue_len(&queue));

    for (int i = 0; i < LEN; ++i) {
        assert(queue_dequeue(&queue, &qdata));
        assert(data[i] == qdata.i32);
    }

    assert(0 == queue_len(&queue));

    queue_clear(&queue);

    fprintf(stderr, "test_len() succeed\n");
}

int main()
{
    test_init();
    test_destroy();
    test_len();
}