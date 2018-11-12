#include <assert.h>
#include <unistd.h>
#include <strings.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

#include "thread_pool.h"


#define LEN 10


void cleanup1(void *args)
{
    UNUSED_PARAM(args);

    fprintf(stderr, "cleanup\n");
}


void *task1(void *args)
{
    int *a = args;

    fprintf(stderr, "%d\n", *a);

    return NULL;
}


void test_pool()
{
    int a[LEN];
    thread_pool_t tp;
    tp_task_t *tasks[LEN];

    bzero(a, sizeof(int) * LEN);

    assert(tp_init(&tp, 20));
    assert(tp_start(&tp));


    for (int i = 0; i < LEN; ++i) {
        tasks[i] = tp_task_create(task1, cleanup1, NULL, 0);
    }

    assert(LEN == tp_post_tasks(&tp, tasks, LEN));

    tp_join_tasks(&tp);
    tp_destroy(&tp);
}


int main()
{
    test_pool();
}