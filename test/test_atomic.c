#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread_pool.h"


#define TASK_NUM 20
#define THREAD_NUM 10


volatile int acnt = 0;
volatile int cnt = 0;


void cleanup(void *args)
{
    UNUSED_PARAM(args);

//    fprintf(stderr, "cleanup++\n");
}


void *f(void *args)
{
    int n;

    UNUSED_PARAM(args);

    for (n = 0; n < 20000; ++n) {
        __sync_add_and_fetch(&acnt, 1); // 原子的
        ++cnt; // 未定义行为，实际上会失去一些更新
    }
    return NULL;
}


void test_atomic()
{
    int i;
    thread_pool_t tp;
    tp_task_t *tasks[TASK_NUM];

    tp_init(&tp, THREAD_NUM);
    tp_start(&tp);

    for (i = 0; i < TASK_NUM; ++i) {
        tasks[i] = tp_task_create(f, cleanup, NULL, 0);
        tp_post_task(&tp, tasks[i]);
    }

//    assert(TASK_NUM == tp_post_tasks(&tp, tasks, TASK_NUM));
    assert(tp_join_tasks(&tp));

    assert(tp.active_tasks == 0);
    assert(queue_isempty(&tp.task_queue));

    fprintf(stderr, "The atomic counter is %u\n", acnt);
    fprintf(stderr, "The non-atomic counter is %u\n", cnt);

    tp_destroy(&tp);
}


int main(void)
{
    test_atomic();

    return 0;
}