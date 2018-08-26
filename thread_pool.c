#include "thread_pool.h"

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>


void *tp_worker(void *args);

void tp_cleanup_unlock(void *args);

void tp_cleanup(void *args);


/* ---------------- Thread Pool API ---------------- */


bool tp_init(thread_pool_t *tp, uint32_t nthreads)
{
    bool status = false;
    bool queue_inited = false;
    bool mutex_inited = false;
    bool cond_inited = false;
    bool threads_allocated = false;

    bzero(tp, sizeof(thread_pool_t));

    tp->nthread = nthreads;

    if (!queue_init(&tp->task_queue)) {
        goto EXIT;
    }

    queue_inited = true;

    if (pthread_mutex_init(&tp->lock, NULL)) {
        perror("pthread_mutex_init() failed");
        goto EXIT;
    }

    mutex_inited = true;

    if (pthread_cond_init(&tp->has_task, NULL)) {
        perror("pthread_cond_init() failed");
        goto EXIT;
    }

    cond_inited = true;

    tp->threads = calloc(nthreads, sizeof(pthread_t));

    if (tp->threads == NULL) {
        perror("failed to allocate threads");
        goto EXIT;
    }

    threads_allocated = true;

    status = true;

EXIT:
    if (!status) {
        if (threads_allocated) {
            free(tp->threads);
        }

        if (cond_inited) {
            if (pthread_cond_destroy(&tp->has_task)) {
                perror("pthread_cond_destroy() failed");
            }
        }

        if (mutex_inited) {
            if (pthread_mutex_destroy(&tp->lock)) {
                perror("pthread_mutex_destroy()");
            }
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


bool tp_post_task(thread_pool_t *tp, tp_task_t *task)
{
    bool status = false;
    tp_task_t *_task = NULL;
    qdata_t data;

    _task = tp_task_create(task->runner, task->cleanup, task->args, task->args_len);

    if (_task == NULL) {
        perror("tp_task_create() failed");
        goto EXIT;
    }

    // todo: check return value?
    data.ptr = _task;
    pthread_mutex_lock(&tp->lock);
    queue_enqueue(&tp->task_queue, data);
    pthread_cond_signal(&tp->has_task);
    pthread_mutex_unlock(&tp->lock);

    status = true;

EXIT:
    return status;
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
                pthread_mutex_lock(&pool->lock);

                pthread_cleanup_push(tp_cleanup_unlock, pool) ;
                        while (queue_isempty(&pool->task_queue)) {
                            pthread_cond_wait(&pool->has_task, &pool->lock);
                        }

                        if (queue_dequeue(&pool->task_queue, &data)) {
                            task = data.ptr;
                        }
                pthread_cleanup_pop(0);

                pthread_mutex_unlock(&pool->lock);

                if (task && task->runner) {
                    if (task->cleanup) {
                        pthread_cleanup_push(task->cleanup, task->args) ;
                                task->runner(task->args);
                        pthread_cleanup_pop(0);
                        task->cleanup(task->args);
                    } else {
                        task->runner(task->args);
                    }

                    tp_task_free(task);
                    task = NULL;
                }
            }

    pthread_cleanup_pop(0);

    return NULL;
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

    if (!tp_task_init(&tls->task, task->runner, task->cleanup,
                      task->args, task->args_len)) {
        goto EXIT;
    }

    tls->key = key;

    status = true;

EXIT:
    if (!status) {
        if (task->runner) {
            if (key_created) {
                pthread_key_delete(key);
            }
        }
    }

    return status;
}


void tp_tls_destroy(thread_local_t *tls)
{
    if (tls) {
        if (tls->data && tls->task.cleanup) {
            tls->task.cleanup(tls->data);
        }

        tp_task_destroy(&tls->task);

        pthread_key_delete(tls->key);
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

    if (tls == NULL) {
        goto EXIT;
    }

    if (tls->data == NULL) {
        tls->data = tls->task.runner(tls->task.args);
    }

    if (tls->data) {
        if (pthread_setspecific(tls->key, tls->data)) {
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
        _args = malloc(args_len);

        if (!_args) {
            goto EXIT;
        }

        memcpy(_args, args, args_len);
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
        if (task->args) {
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

