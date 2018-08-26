#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "thread_pool.h"


void *task1(void *args);

void *task2(void *args);


void *task1(void *args)
{
    thread_pool_t *tp;
    tp_task_t *task;

    tp = tp_self();
    fprintf(stderr, "task1 tp: %p\n", tp);

    task = tp_task_create(task2, NULL, NULL, 0);
    tp_post_task(tp, task);

    return NULL;
}


void *task2(void *args)
{
    thread_pool_t *tp;

    tp = tp_self();

    fprintf(stderr, "task2 tp: %p\n", tp);

    return NULL;
}


void test_self()
{
    thread_pool_t tp;
    tp_task_t *task;

    fprintf(stderr, "tp: %p\n", &tp);

    assert(tp_init(&tp, 5));
    assert(tp_start(&tp));

    task = tp_task_create(task1, NULL, NULL, 0);
    tp_post_task(&tp, task);

    sleep(2);

    thread_pool_t *p;
    p = tp_self();
    fprintf(stderr, "main thread tp: %p\n", p);
//    tp_join(&tp);
    tp_destroy(&tp);
}


int main()
{
    test_self();
}