#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/**
 * Thread Pool based on pthread implementing in `C`.
 *
 * Usage:
 *
 */

#include <pthread.h>

#include "queue.h"


typedef void *(*runnable_t)(void *args);

typedef void (*cleanup_t)(void *args);

typedef struct tp_task_s tp_task_t;
typedef struct thread_pool_s thread_pool_t;
typedef struct thread_local_s thread_local_t;


struct tp_task_s
{
    runnable_t runner;
    cleanup_t cleanup;
    void *args;
    size_t args_len;
};


struct thread_pool_s
{
    uint32_t nthread;
    pthread_t *threads;
    queue_t task_queue;
    pthread_mutex_t lock;
    pthread_cond_t has_task;

    uint32_t active_tasks;
    pthread_cond_t no_task;
};


struct thread_local_s
{
    pthread_key_t key;
    tp_task_t *task;
};


/* ---------------- Thread Pool API ---------------- */


/**
 * Initialize a thread pool. No threads would be created or
 * started in this routine.
 *
 * @param tp thread pool to be initialized
 * @param nthreads number of threads to be created in pool
 * @return true: succeed
 *         false: failed
 */
bool tp_init(thread_pool_t *tp, uint32_t nthreads);


/**
 * Start an initialized thread pool. Threads will be created
 * and started in this routine.
 *
 * @param tp non-started thread pool
 * @return true: succeed
 *         false: failed
 */
bool tp_start(thread_pool_t *tp);


/**
 * Destroy an thread pool.
 *
 * @param tp thread pool to be destroy
 */
void tp_destroy(thread_pool_t *tp);


/**
 * Waiting for all threads in pool to stop.
 *
 * @param tp started thread pool
 */
void tp_join(thread_pool_t *tp);


//bool tp_shutdown(thread_pool_t *tp);


/**
 * Post an task to the thread pool. The task would
 * be queued and waiting for threads consuming it.
 *
 * @param tp started thread pool
 * @param task an task in heap, which would be released
 *        by the pool after it's been consumed
 * @return true: succeed
 *         false: failed
 */
bool tp_post_task(thread_pool_t *tp, tp_task_t *task);


/**
 * Waiting for all currently running tasks to finish.
 *
 * @param tp thread pool
 * @return true: there is no running tasks now
 *         false: there are new tasks added just before
 *                tp_join_tasks() return
 */
bool tp_join_tasks(thread_pool_t *tp);


/**
 * Post all tasks as a batch, which means it's a atomic action.
 *
 * If you call tp_post_task() in a loop to post tasks one by one,
 * it's possible that before you post the second task the first
 * task has been finished.
 * However, it doesn't matter in most situations,
 * just take it simply as a repetition of tp_post_task().
 *
 * @param tp thread pool
 * @param tasks array of tasks
 * @param ntask number of tasks in the array
 * @return number of successfully posted tasks
 */
int tp_post_tasks(thread_pool_t *tp, tp_task_t *tasks[], int ntask);


/**
 * In task function, get the thread local itself.
 * @return thread local itself or NULL when error occurred
 */
thread_pool_t *tp_self();


/* ---------------- Task API ---------------- */


/**
 * Initialize an task structure.
 *
 * @param task task to be initialized
 * @param runner the running part of the task
 * @param cleanup the cleanup part of the task
 * @param args the arguments to be passed to the `runner` & `cleanup`
 * @param args_len the memory size in bytes of the memory area that the args pointing to,
 *                 which will be copied to the inner allocated memory area
 * @return true: succeed
 *         false: failed
 */
bool tp_task_init(tp_task_t *task, runnable_t runner, cleanup_t cleanup,
                  void *args, size_t args_len);


/**
 * Uninitialize and task structure, freeing the inner allocated memories.
 *
 * @param task task to be uninitialized
 */
void tp_task_destroy(tp_task_t *task);


/**
 * Create an task.
 * Allocating the tp_task_t structure and initialize it.
 *
 * @param runner the running part of the task
 * @param cleanup the cleanup part of the task
 * @param args the arguments to be passed to the `runner` & `cleanup`
 * @param args_len the memory size in bytes of the memory area that the args pointing to,
 *                 which will be copied to the inner allocated memory area
 * @return task created or NULL when failed
 */
tp_task_t *tp_task_create(runnable_t runner, cleanup_t cleanup,
                          void *args, size_t args_len);


/**
 * Free an task.
 * Uninitialize it and free the memory.
 *
 * @param task task to be freed
 */
void tp_task_free(tp_task_t *task);


/**
 * Create a new task and copy the data of the old task to the new one.
 *
 * @param task the old task to be copied
 * @return task created or NULL when failed
 */
#define tp_task_create_copy(task) \
tp_task_create((task)->runner, (task)->cleanup, (task)->args, (task)->args_len)


/* ---------------- Thread Local API ---------------- */


/**
 * Initialize a thread local storage.
 *
 * @param tls thread local storage to be initialized
 * @param task task for the creation and cleanup of the value,
 *             which must be allocated in heap and would be freed
 *             when tls destroyed
 * @return true: succeed
 *         false: failed
 */
bool tp_tls_init(thread_local_t *tls, tp_task_t *task);


/**
 * Destroy a thread local storage.
 *
 * @param tls thread local storage to be destroyed
 */
void tp_tls_destroy(thread_local_t *tls);


/**
 * Get the value of the thread local storage.
 *
 * @param tls thread local storage
 * @return the pointer of the value
 */
void *tp_get_tls(thread_local_t *tls);


/**
 * Set the value of the thread local storage.
 * Actually the value is created by the associated task,
 * so no argument passed as the new value.
 *
 * Currently it's not supported to change the tls value,
 * because it's an strange behavior.
 *
 * @param tls thread local storage
 * @return true: succeed
 *         false: failed
 */
bool tp_set_tls(thread_local_t *tls);


#endif //THREAD_POOL_H
