# Thread Pool in C

Thread Pool based on pthread implemented in C.



## Usage

### pool operations

```c
const int THREAD_NUM = 10;
thread_pool_t tp;

// Initialize an thread pool
tp_init(&tp, THREAD_NUM);

// Start an thread pool
tp_start(&tp);

// Join the thread pool, waiting until all thread ended
tp_join(&tp);

// Join the running tasks, waiting until all running tasks finished
tp_join_tasks(&tp);

// Destroy the thread pool
tp_destroy(&tp);

// Get the thread_pool_t itself in the task function
tp_self();
```



### task operations

`tp_task_t` represent a task which could be executed and cleanup.

```c
void cleanup1(void *args);
void *task1(void *args);

int *args;
tp_task_t *task;
thread_pool_t tp;

// Initialize a task
task = tp_task_create(task1, cleanup1, args, sizeof(int));
    
// Post a task to the thread pool `tp`
tp_post_task(&tp, task);

// Free a task.
// DO NOT call this function to free a task.
// Task is created by tp_task_create() which allocate memory in heap for it.
// After a task was posted, users have no way to detect whether the task has 
// been finished. Therefore, tasks' deconstruction is delegated to the pool,
// which will call tp_task_free() to destroy a task after it's been finished.
tp_task_free(task);
```



batch posting:

```c
#define TASK_NUM 10

tp_task_t *tasks[TASK_NUM];

for (int i = 0; i < TASK_NUM; ++i) {
    tasks[i] = tp_task_create(task1, cleanup1, args, sizeof(int));
    // post task in loop
    // tp_task_post(&tp, tasks[i]);
}

// Post all tasks as a batch, which means it's a atomic action.
// If you call tp_post_task() in a loop to post tasks one by one,
// it's possible that before you post the second task the first 
// task has been finished.
// 
// Note:
// Tasks posted to the pool would be released after it's been finished,
// but the task array won't.
tp_post_tasks(&tp, tasks, TASK_NUM);
```



### thread local storage

`thread_local_t` is a key for an thread local storage. 

In task function, using `tp_get_tls()` with the key to get the value. 

The value was created once by `tls_create`, and will be cleanup when the thread was ended by `tls_cleanup`.

```c
void *tls_create(void *args);
void tls_cleanup(void *args);

thread_pool_t tp;
tp_task_t *task;
thread_local_t tls;

// Initialize a thread local task, used to create and cleanup 
// the variables for each thread.
task = tp_task_create(tls_create, tls_cleanup, NULL, 0);
tp_tls_init(&tls, task);

// In task function
thread_local_t *tls = args->tls;
void *data = tp_get_tls(tls);
// Then you can consume the returned data.

// Destroy a thread local storage, which also free the task it holds.
tp_tls_destroy(&tls);
```



### queue operations

`queue_t` is a FIFO queue.

`qdata_t` is the data wrapper for the queue, which is an union type of `int32_t`, `uint8_t`,  `void *`, etc.

```c
qdata_t qdata;
queue_t queue;

// Initialize a queue
queue_init(&queue);
// Clear a queue
queue_clear(&queue);

// Enqueue an 32-bit interger number
qdata.i32 = 1;
queue_enqueue(&queue, qdata);

// Dequeue a qdata number
qdata_t qdata2;
queue_dequeue(&queue, &qdata2);
printf("%d\n", qdata2.i32);
```