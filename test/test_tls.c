#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "thread_pool.h"


#define LEN 10


void *tls_create(void *args)
{
    fprintf(stderr, "tls_create():%lu %p\n", pthread_self(), args);

    int *i = calloc(1, sizeof(int));
    return i;
}


void tls_cleanup(void *args)
{
    int *i = args;

    fprintf(stderr, "tls_cleanup():%lu %d\n", pthread_self(), *i);

    if (args) {
        free(args);
    }
}


void *task_func(void *args)
{
    int i;
    thread_local_t *tls = args;

    int *data = tp_get_tls(tls);

    for (i = 0; i < 3; ++i) {
        *data += 2 + *data;
//        fprintf(stderr, "[%lu] %d\n", pthread_self(), *data);
    }

    sleep(1);

    return NULL;
}


void test_tls()
{
    int i;
    thread_pool_t tp;
    tp_task_t *task;
    thread_local_t tls;

    assert(tp_init(&tp, 5));
    assert(tp_start(&tp));

    task = tp_task_create(tls_create, tls_cleanup, NULL, 0);
    assert(task);
    assert(tp_tls_init(&tls, task));

    for (i = 0; i < LEN; ++i) {
        task = tp_task_create(task_func, NULL, &tls, 0);
        assert(task);
        tp_post_task(&tp, task);
    }

    tp_join_tasks(&tp);
    tp_destroy(&tp);
}


int main()
{
    test_tls();

    return 0;
}
