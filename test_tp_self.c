#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "thread_pool.h"


void *task1(void *args);

void *task2(void *args);


thread_pool_t g_tp;


void *task1(void *args)
{
    thread_pool_t *tp;
    tp_task_t *task;

    tp = tp_self();
    fprintf(stderr, "task1 tp: %p\n", tp);

    task = tp_task_create(task2, NULL, NULL, 0);
    tp_post_task(tp, task);

    assert(&g_tp == tp);

    return NULL;
}


void *task2(void *args)
{
    thread_pool_t *tp;

    tp = tp_self();

    fprintf(stderr, "task2 tp: %p\n", tp);

    assert(&g_tp == tp);

    return NULL;
}


void test_self()
{
    tp_task_t *task;

    fprintf(stderr, "tp: %p\n", &g_tp);

    assert(tp_init(&g_tp, 5));
    assert(tp_start(&g_tp));

    task = tp_task_create(task1, NULL, NULL, 0);
    tp_post_task(&g_tp, task);

    thread_pool_t *p;
    p = tp_self();

    fprintf(stderr, "main thread tp: %p\n", p);

    assert(&g_tp == p);

    tp_join_tasks(&g_tp);
    tp_destroy(&g_tp);
}


int main()
{
    test_self();
}