#include "./init.h"
#include <pthread.h>
#include "../clientserver/clientserver.h"

int server_fd;

int INIT_HEALTH_CHECK_jobs(PromiseStore *store){
    // HELTH CHECK INIT (FUTURE) 
}
int INIT_health_cleaner_threads(
    unsigned int thread_count, 
    unsigned int shared_memory_size,
    double threshold,
    double min_threshold,
    double max_threshold,
    PromiseStore *store,
    void *(health_rootine)(void *args),
    void *(cleaner_rootine)(void *args)
){
    if (shared_memory_size < 1){
        printf("shared memory size is too small, at least >= 1\n");
        return -1;
    }

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
    // health checker thread
    pthread_t *health_thread = calloc(1, sizeof(pthread_t));
    if (!health_thread){
        free_promise_store(store);
        free(args);
        free(cleaner_thread);
        printf("can't allocate health thread\n");
        return -1;
    }
    printf("[+]cleaner thread allocated\n");
    printf("[+]health thread allocated\n");
    // start threads
    pthread_create(cleaner_thread, NULL, cleaner_rootine, args);
    printf("[+]created cleaner threads\n");
    pthread_create(health_thread, NULL, health_rootine, args);
    printf("[+]created health threads\n");
    sleep(5);
    args->stop_flag = true;
    printf("[*]flag is switched to true\n");
    // run threads detached
    pthread_detach(*cleaner_thread);
    pthread_detach(*health_thread);
    printf("[+]joined threads\n");

    // clean
    // free_promise_store(store);
    // free(cleaner_thread);
    // free(args);
    printf("[+]free\n");
    return (0);
}

/**
 * function that spawns threads to handle incoming 
 * requests
 */
void Init_Server_multithread(PromiseStore *store) {
    // set up socket
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);
    unlink(SOCKET_PATH);
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    // bind
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    // listen to up 5 connections
    listen(server_fd, 50);
    printf("[v] server is listening on 127.0.0.1\n");
    while (true) {
        // accept incoming client 
        server_arg *ser_args = calloc(1, sizeof(server_arg));
        ser_args->client_fd = accept(server_fd, NULL, NULL);
        ser_args->store = store;
        // on error
        if (ser_args->client_fd < 0) { perror("accept"); free(ser_args); continue; }
        // spawn a handler thread
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, ser_args);
        //pthread_join(tid, NULL);
        pthread_detach(tid); // don't need to join
    }
}