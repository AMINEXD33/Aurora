#include "./init.h"
#include <pthread.h>

int INIT_HEALTH_CHECK_jobs(){
    // HELTH CHECK INIT (FUTURE) 
}
int INIT_COM_jobs(
    unsigned int thread_count, 
    unsigned int shared_memory_size,
    double threshold,
    double min_threshold,
    double max_threshold,
    void *(server_rootine)(void *args),
    void *(cleaner_rootine)(void *args)
){
    if (shared_memory_size < 1){
        printf("shared memory size is too small, at least >= 1\n");
        return -1;
    }

    // create promise store
    PromiseStore* store = InitPromiseStore(
        shared_memory_size,
        threshold,
        min_threshold,
        max_threshold
    );
    if (!store){
        printf("can't allocate promise store\n");
        return -1;
    }
    printf("[+]store allocated\n");
    // thread args
    thread_args *args = malloc(sizeof(thread_args));
    if (!args){
        printf("can't allocate thread args\n");
        free_promise_store(store);
        return -1;
    }
    args->stop_flag = false;
    args->store = store;
    printf("[+]args allocated\n");
    // cleaner thread
    pthread_t *cleaner_thread = calloc(1 , sizeof(pthread_t));
    if (!cleaner_thread){
        free_promise_store(store);
        free(args);
        printf("can't allocate cleaner thread\n");
        return -1;
    }
    printf("[+]cleaner thread allocated\n");
    // start threads
    pthread_create(cleaner_thread, NULL, cleaner_rootine, args);
    printf("[+]created threads\n");
    sleep(5);
    args->stop_flag = true;
    printf("[*]flag is switched to true\n");
    // join threads
    pthread_join(*cleaner_thread, NULL);
    printf("[+]joined threads\n");

    Server_init_multithread();

    // clean
    free_promise_store(store);
    free(cleaner_thread);
    free(args);
    printf("[+]free\n");
    return (0);
}