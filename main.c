#include "./engine/core/init/init.h"
#include "./engine/core/jobs/jobs.h"
#include "./engine/helpers/helpers.h"
#include "./engine/core/config/config.h"
#include "./engine/core/clientserver/clientserver.h"
#include <stdio.h>      // for printf, perror
#include <stdlib.h>     // for exit
#include <unistd.h>     // for fork, getpid, getppid
#include <sys/wait.h> 
int main (char **argc, int argv){
    cJSON *core_config =  INIT_CORE_CONFIG();
    int thread_count = GET_THREAD_COUNT(core_config);
    int core_count = GET_CORE_COUNT(core_config);
    double threshold = GET_THRESHOLD(core_config);
    double min_threshold = GET_MIN_THRESHOLD(core_config);
    double max_threshold = GET_MAX_THRESHOLD(core_config);
    int shared_mem_unites = GET_SHARED_MEMORY_UNITES(core_config);
    
    printf("---------LOADED CONFIG-----------\n");
    printf("[@] thread count = %d\n", thread_count);
    printf("[@] core count = %d\n", core_count);
    printf("[@] threshold = %lf\n", threshold);
    printf("[@] min threshold = %lf\n", min_threshold);
    printf("[@] max threshold = %lf\n", max_threshold);
    printf("[@] shared memory unites  = %d\n", shared_mem_unites);
    printf("---------------------------------\n");


    // create promise store
    PromiseStore* store = InitPromiseStore(
        shared_mem_unites,
        threshold,
        min_threshold,
        max_threshold
    );
    if (!store){
        printf("can't allocate promise store\n");
        return -1;
    }
    printf("[+]store allocated\n");
    // FORK server process 
    pid_t p = fork();
    if (p < 0){
        printf("failed to fork server\n");
    }
    if (p == 0){
        // init the health and cleaner threads
        INIT_health_cleaner_threads(
            thread_count,
            shared_mem_unites,
            threshold,
            min_threshold,
            max_threshold,
            store,
            health_thread,
            cleaner_thread
        );
        // start the server
        Init_Server_multithread(store);
        exit(0);
    }

    // FORK worker processes
    for (unsigned int i = 0; i < core_count; i++) {
        pid_t p = fork();
        if (p < 0) {
            perror("fork failed\n");
            exit(1);
        }
        if (p == 0) {
            // child
            printf("[child %u] pid=%d ppid=%d\n", i, getpid(), getppid());
            // SEND some stuff every 10 seconds
            while (true){
                sendstuff();
                sleep(1);
            }
            exit(0); // IMPORTANT: prevent it from re-running the loop
        }
        // parent continues to next iteration
        // wait(NULL);
    }
    // HELTH CHECKS STUFF
    return 1;
}