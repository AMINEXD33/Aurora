#include "./init.h"
#include <pthread.h>
#include "../clientserver/clientserver.h"

int server_fd;

int INIT_HEALTH_CHECK_jobs(PromiseStore *store){
    // HELTH CHECK INIT (FUTURE) 
}
int INIT_health_cleaner_threads(
    unsigned int thread_count, 
    void *(health_rootine)(void *args)
){
    // thread args
    thread_args *args = malloc(sizeof(thread_args));
    if (!args){
        printf("can't allocate thread args\n");
        return -1;
    }
    args->stop_flag = false;
    printf("[+]args allocated\n");
    // cleaner thread
    // health checker thread
    pthread_t *health_thread = calloc(1, sizeof(pthread_t));
    if (!health_thread){
        free(args);
        printf("can't allocate health thread\n");
        return -1;
    }
    printf("[+]health thread allocated\n");
    // start threads
    pthread_create(health_thread, NULL, health_rootine, args);
    printf("[+]created health thread\n");
    // run threads detached
    pthread_detach(*health_thread);
    // clean
    // free_promise_store(store);
    // free(cleaner_thread);
    // free(args);
    // printf("[+]free\n");
    return (0);
}
