#include "thread_pool.h"

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct
{
    thread_pool_t *tp;
} tp_self_arg;


void _tp_self_key_init(thread_pool_t *tp);

void _tp_self_destroy(thread_pool_t *tp);

void *tp_worker(void *args);

void tp_cleanup_unlock(void *args);

void tp_cleanup(void *args);


static thread_local_t g_self_tls;
static bool g_self_key_inited = false;


/* ---------------- Thread Pool API ---------------- */


bool _tp_init_pthread_vars(thread_pool_t *tp)
{
    bool status = false;

    bool lock_inited = false;
    bool cond_inited = false;
    bool no_task_inited = false;

    if (pthread_mutex_init(&tp->lock, NULL)) {
        perror("pthread_mutex_init() for `lock` failed");
        goto EXIT;
    }

    lock_inited = true;

    if (pthread_cond_init(&tp->has_task, NULL)) {
        perror("pthread_cond_init() for `has_task` failed");
        goto EXIT;
    }

    cond_inited = true;

    if (pthread_cond_init(&tp->no_task, NULL)) {
        perror("pthread_cond_init() for `no_task` failed");
        goto EXIT;
    }

    no_task_inited = true;

    status = true;

EXIT:
    if (!status) {
        if (no_task_inited) {
            if (pthread_cond_destroy(&tp->no_task)) {
                perror("pthread_cond_destroy() for `no_task` failed");
            }
        }

        if (cond_inited) {
            if (pthread_cond_destroy(&tp->has_task)) {
                perror("pthread_cond_destroy() for `has_task` failed");
            }
        }

        if (lock_inited) {
            if (pthread_mutex_destroy(&tp->lock)) {
                perror("pthread_mutex_destroy() for `lock` failed");
            }
        }
    }
    return status;
}


bool tp_init(thread_pool_t *tp, uint32_t nthreads)
{
    bool status = false;
    bool queue_inited = false;
    bool threads_allocated = false;

    bzero(tp, sizeof(thread_pool_t));

    tp->nthread = nthreads;

    if (!queue_init(&tp->task_queue)) {
        goto EXIT;
    }

    queue_inited = true;

    if (!_tp_init_pthread_vars(tp)) {
        goto EXIT;
    }

    tp->threads = calloc(nthreads, sizeof(pthread_t));

    if (tp->threads == NULL) {
        perror("failed to allocate threads");
        goto EXIT;
    }

    threads_allocated = true;

    _tp_self_key_init(tp);

    status = true;

EXIT:
    if (!status) {
        if (threads_allocated) {
            free(tp->threads);
        }

        if (queue_inited) {
            queue_clear(&tp->task_queue);
        }

        bzero(tp, sizeof(thread_pool_t));
    }

    return status;
}


bool tp_start(thread_pool_t *tp)
{
    bool status = false;
    int threads_created_num = 0;

    if (tp == NULL || tp->threads == NULL) {
        goto EXIT;
    }

    for (int i = 0; i < tp->nthread; ++i) {
        if (pthread_create(&tp->threads[i], NULL, tp_worker, tp)) {
            threads_created_num = i;
            goto EXIT;
        }
    }

    status = true;

EXIT:
    if (!status) {
        for (int i = 0; i < threads_created_num; ++i) {
            if (pthread_cancel(tp->threads[i])) {
                perror("pthread_cancel() failed");
            }
        }

        for (int i = 0; i < threads_created_num; ++i) {
            if (pthread_join(tp->threads[i], NULL)) {
                perror("pthread_join() failed");
            }
        }
    }

    return status;
}


void tp_destroy(thread_pool_t *tp)
{
    int i;

    if (tp == NULL) {
        return;
    }

    if (tp->threads) {
        for (i = 0; i < tp->nthread; ++i) {
            // todo: check return value
            if (pthread_cancel(tp->threads[i])) {
                perror("pthread_cancel() failed");
            }
        }

        tp_join(tp);

        free(tp->threads);
    }

    if (pthread_cond_destroy(&tp->has_task)) {
        perror("pthread_cond_destroy() failed");
    }

    if (pthread_mutex_destroy(&tp->lock)) {
        perror("pthread_mutex_destroy()");
    }

    if (g_self_key_inited) {
        _tp_self_destroy(tp);
        g_self_key_inited = false;
    }

    queue_clear(&tp->task_queue);

    bzero(tp, sizeof(thread_pool_t));
}


void tp_join(thread_pool_t *tp)
{
    if (tp && tp->threads) {
        for (int i = 0; i < tp->nthread; ++i) {
            // todo: check return value
            if (pthread_join(tp->threads[i], NULL)) {
                perror("pthread_join() failed");
            }
        }
    }
}


bool tp_join_tasks(thread_pool_t *tp)
{
    pthread_mutex_lock(&tp->lock);

    do {
        pthread_cond_wait(&tp->no_task, &tp->lock);
    } while (tp->active_tasks > 0);

    pthread_mutex_unlock(&tp->lock);

    return tp->active_tasks == 0;
}


bool tp_post_task(thread_pool_t *tp, tp_task_t *task)
{
    bool status = false;
    bool posted = false;
    qdata_t data;

    if (tp == NULL || task == NULL) {
        goto EXIT;
    }

    // todo: check return value?
    data.ptr = task;
    pthread_mutex_lock(&tp->lock);
    posted = queue_enqueue(&tp->task_queue, data);
    pthread_mutex_unlock(&tp->lock);

    if (!posted) {
        goto EXIT;
    }

    __sync_add_and_fetch(&tp->active_tasks, 1);
    __sync_synchronize();

    pthread_cond_signal(&tp->has_task);

    status = true;

EXIT:
    return status;
}


int tp_post_tasks(thread_pool_t *tp, tp_task_t *tasks[], int ntask)
{
    int posted = 0;
    qdata_t data;

    if (tp == NULL || tasks == NULL || ntask == 0) {
        goto EXIT;
    }

    pthread_mutex_lock(&tp->lock);
    for (int i = 0; i < ntask; ++i) {
        data.ptr = tasks[i];
        if (queue_enqueue(&tp->task_queue, data)) {
            ++posted;
        }
    }
    pthread_mutex_unlock(&tp->lock);

    if (posted) {
        __sync_add_and_fetch(&tp->active_tasks, ntask);
        __sync_synchronize();

        pthread_cond_signal(&tp->has_task);
    }

EXIT:
    return posted;
}


void tp_cleanup_unlock(void *args)
{
    thread_pool_t *pool = args;

    pthread_mutex_unlock(&pool->lock);
}


void tp_cleanup(void *args)
{
    fprintf(stderr, "thread ended\n");
}


void *tp_worker(void *args)
{
    qdata_t data;
    thread_pool_t *pool = args;
    tp_task_t *task = NULL;

    pthread_cleanup_push(tp_cleanup, pool) ;

            while (1) {
                data.ptr = NULL;
                task = NULL;

                // Take a task
                pthread_mutex_lock(&pool->lock);

                pthread_cleanup_push(tp_cleanup_unlock, pool) ;
                        // If no task in queue, wait until someone post one.
                        while (queue_isempty(&pool->task_queue)) {
                            pthread_cond_wait(&pool->has_task, &pool->lock);
                        }
                        // Dequeue a task for running
                        if (queue_dequeue(&pool->task_queue, &data)) {
                            task = data.ptr;
                        }

                pthread_cleanup_pop(0);

                pthread_mutex_unlock(&pool->lock);

                // Run a task
                if (task && task->runner) {
                    if (task->cleanup) {
                        pthread_cleanup_push(task->cleanup, task->args) ;
                                task->runner(task->args);
                        pthread_cleanup_pop(0);
                        task->cleanup(task->args);
                    } else {
                        task->runner(task->args);
                    }


                    // Note: pool->task_queue is empty DO NOT means there is no task
                    //
                    // If there is no task remain in the queue after dequeue operation,
                    // signal for tp_join_task()
                    if (__sync_sub_and_fetch(&pool->active_tasks, 1) == 0) {
                        pthread_cond_signal(&pool->no_task);
                    }

                    tp_task_free(task);
                    task = NULL;
                }
            }

    pthread_cleanup_pop(0);

    return NULL;
}


/* ---------------- Thread Pool Self API ---------------- */


void *_tp_self_create(void *args)
{
    tp_self_arg *arg = NULL;

    arg = malloc(sizeof(tp_self_arg));

    if (arg) {
        arg->tp = args;
    }

    return arg;
}


#define _tp_self_cleanup free


void _tp_self_key_init(thread_pool_t *tp)
{
    tp_task_t *task;

    // pass `tp` to it directly without copying by setting
    // args_len to 0.
    task = tp_task_create(_tp_self_create, _tp_self_cleanup, tp, 0);

    if (!task) {
        goto EXIT;
    }

    if (!tp_tls_init(&g_self_tls, task)) {
        goto EXIT;
    }


    if (!tp_set_tls(&g_self_tls)) {
        goto EXIT;
    }

    g_self_key_inited = true;

EXIT:
    return;
}


void _tp_self_destroy(thread_pool_t *tp)
{
    if (g_self_key_inited) {
        tp_tls_destroy(&g_self_tls);
    }
}


thread_pool_t *tp_self()
{

    tp_self_arg *arg = NULL;
    thread_pool_t *pool = NULL;

    if (g_self_key_inited) {
        arg = tp_get_tls(&g_self_tls);
        pool = arg->tp;
    }

    return pool;
}


/* ---------------- Thread Local API ---------------- */


bool tp_tls_init(thread_local_t *tls, tp_task_t *task)
{
    bool status = false;
    bool key_created = false;
    pthread_key_t key = 0;

    if (task == NULL || task->runner == NULL) {
        goto EXIT;
    }

    if (pthread_key_create(&key, task->cleanup)) {
        perror("pthread_key_create() failed");
        goto EXIT;
    }

    key_created = true;

    tls->task = task;
    tls->key = key;

    status = true;

EXIT:
    if (!status) {
        if (key_created) {
            pthread_key_delete(key);
        }
    }

    return status;
}


void tp_tls_destroy(thread_local_t *tls)
{
    if (tls) {
        if (tls->task) {
            tp_task_free(tls->task);
        }

        pthread_key_delete(tls->key);

        bzero(tls, sizeof(thread_local_t));
    }
}


void *tp_get_tls(thread_local_t *tls)
{
    void *data = NULL;

    if (tls == NULL) {
        goto EXIT;
    }

    data = pthread_getspecific(tls->key);

    if (data == NULL) {
        if (!tp_set_tls(tls)) {
            goto EXIT;
        }

        data = pthread_getspecific(tls->key);

        if (data == NULL) {
            goto EXIT;
        }
    }

EXIT:
    return data;
}


bool tp_set_tls(thread_local_t *tls)
{
    bool status = false;
    void *data = NULL;

    if (tls == NULL) {
        goto EXIT;
    }

    data = pthread_getspecific(tls->key);

    if (data == NULL) {
        data = tls->task->runner(tls->task->args);
    }

    if (data) {
        if (pthread_setspecific(tls->key, data)) {
            perror("pthread_setspecific() failed");
            goto EXIT;
        }
    }

    status = true;

EXIT:
    return status;
}


/* ---------------- Task API ---------------- */


bool tp_task_init(tp_task_t *task,
                  runnable_t runner,
                  cleanup_t cleanup,
                  void *args,
                  size_t args_len)
{
    bool status = false;
    void *_args = NULL;

    if (!task) {
        goto EXIT;
    }

    if (args) {
        // Do not copy when args_len == 0
        if (args_len == 0) {
            _args = args;
        } else {
            _args = malloc(args_len);

            if (!_args) {
                goto EXIT;
            }

            memcpy(_args, args, args_len);
        }
    }

    task->runner = runner;
    task->cleanup = cleanup;
    task->args = _args;
    task->args_len = args_len;
    status = true;

EXIT:
    return status;
}


void tp_task_destroy(tp_task_t *task)
{
    if (task) {
        // if args_len == 0 then don't free
        // cause it didn't be allocated
        if (task->args && task->args_len) {
            free(task->args);
        }

        memset(task, 0, sizeof(tp_task_t));
    }
}


tp_task_t *tp_task_create(runnable_t runner, cleanup_t cleanup,
                          void *args, size_t args_len)
{
    tp_task_t *task = NULL;

    task = malloc(sizeof(tp_task_t));

    tp_task_init(task, runner, cleanup, args, args_len);

    return task;
}


void tp_task_free(tp_task_t *task)
{
    if (task) {
        tp_task_destroy(task);

        free(task);
    }
}

