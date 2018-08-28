#include <assert.h>
#include <unistd.h>
#include <strings.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

#include "thread_pool.h"


#define LEN 10


struct _args_s
{
    int *num;
    int index;
};


void cleanup1(void *args)
{
    struct _args_s _args;

    memcpy(&_args, args, sizeof(struct _args_s));

    fprintf(stderr, "cleanup [%d]: %d\n", _args.index, *_args.num);

    assert(*_args.num == _args.index);
}


void *task1(void *args)
{
    struct _args_s _args;

    memcpy(&_args, args, sizeof(struct _args_s));

    while (*_args.num < _args.index) {
        ++(*_args.num);
    }


    return NULL;
}


void test_pool()
{
    int a[LEN];
    thread_pool_t tp;
    struct _args_s args;
    tp_task_t *tasks[LEN];

    bzero(a, sizeof(int) * LEN);

    assert(tp_init(&tp, 20));
    assert(tp_start(&tp));


    for (int i = 0; i < LEN; ++i) {
        args.num = &a[i];
        args.index = i;

        tasks[i] = tp_task_create(task1, cleanup1,
                              &args, sizeof(struct _args_s));
    }

    assert(LEN == tp_post_tasks(&tp, tasks, LEN));

    tp_join_tasks(&tp);
    tp_destroy(&tp);
}


int main()
{
    test_pool();
}