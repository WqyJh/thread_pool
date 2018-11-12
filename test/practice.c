#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <thread_pool.h>


pthread_key_t key;


void tls_cleanup(void *args)
{
    UNUSED_PARAM(args);

    fprintf(stderr, "tls_cleanup()\n");
}


void *func(void *args)
{
    UNUSED_PARAM(args);

    int i = 0;
    int j = 0;

    pthread_setspecific(key, &i);

    usleep(1000 * 1000);

    printf("func(): %p\n", pthread_getspecific(key));

    pthread_setspecific(key, &j);

    usleep(1000 * 1000);


    printf("func(): %p\n", pthread_getspecific(key));

    return NULL;
}


void _test()
{
    pthread_t t;

    pthread_key_create(&key, tls_cleanup);
    pthread_create(&t, NULL, func, NULL);
    pthread_join(t, NULL);
}


int main()
{
    _test();
}