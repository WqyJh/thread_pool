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
    fprintf(stderr, "tls_cleanup():%lu %p\n", pthread_self(), args);

    if (args) {
        free(args);
    }
}


void *task_func(void *args)
{
    thread_local_t *tls = args;

    int *data = tp_get_tls(tls);

    for (int i = 0; i < 3; ++i) {
        *data += 2 + *data;
        fprintf(stderr, "[%lu] %d\n", pthread_self(), *data);
        sleep(1);
    }

    return NULL;
}


void test_tls()
{
    thread_pool_t tp;
    tp_task_t task;
    thread_local_t tls;

    assert(tp_init(&tp, 5));
    assert(tp_start(&tp));

    tp_task_init(&task, tls_create, tls_cleanup, NULL, 0);
    tp_tls_init(&tls, &task);
    tp_task_destroy(&task);

    for (int i = 0; i < LEN; ++i) {
        tp_task_init(&task, task_func, NULL,
                     &tls, sizeof(thread_local_t));
        tp_post_task(&tp, &task);
        tp_task_destroy(&task);
    }

    sleep(4);
    tp_destroy(&tp);
//    tp_join(&tp);
    tp_task_destroy(&task);
    tp_tls_destroy(&tls);
}


int main()
{
    test_tls();
    return 0;
}
