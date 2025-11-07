#include "../init/init.h"

/**
 * the cleaner thread , updates the threshold and frees 
 * cache that is bellow that threshold ofc if it's not used 
 * or awaited by an other thread
 * 
 * ### return:
 *  `void *`
 */
void *worker_thread(void *arg){
    thread_args *args = (thread_args *)arg;
    PromiseStore *store = args->store;
    printf("[+]working thread is UP\n");



    return NULL;
}