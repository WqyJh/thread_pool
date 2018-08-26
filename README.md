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

// Join the thread pool
tp_join(&tp);

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
tp_task_t task;
thread_pool_t tp;

// Initialize a task
tp_task_init(&task, task1, cleanup1, args, sizeof(int));
             
// Post a task to the thread pool `tp`
tp_post_task(&tp, &task);
```



### thread local storage

`thread_local_t` is a key for an thread local storage. 

In task function, using `tp_get_tls()` with the key to get the value. 

The value was created once by `tls_create`, and will be cleanup when the thread was ended by `tls_cleanup`.

```c
void *tls_create(void *args);
void tls_cleanup(void *args);

thread_pool_t tp;
tp_task_t task;
thread_local_t tls;

tp_task_init(&task, tls_create, tls_cleanup, NULL, 0);
tp_tls_init(&tls, &task);

// In task function
thread_local_t *tls = args->tls;
void *data = tp_get_tls(tls);
// Then you can consume the returned data.
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