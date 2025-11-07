#include "../init/init.h"
#include "../../helpers/helpers.h"
/**
 * the cleaner thread , updates the threshold and frees 
 * cache that is bellow that threshold ofc if it's not used 
 * or awaited by an other thread
 * 
 * ### return:
 *  `void *`
 */
void *cleaner_thread(void *arg){
    thread_args *args = (thread_args *)arg;
    PromiseStore *store = args->store;
    // see delta of last clean
    while(true){
        if (args->stop_flag){
            break;
        }
        double occupancy = ((double)store->count / (double)store->capacity) * 100;
        // don't start cleaning when the occuancy is bellow 50%
        if (occupancy < 50){
            sleep(0.1);
        }
        // lock the store
        pthread_mutex_lock(&store->lock);
        // get the new threshold
        double threshold = update_store_threshold(store);
        // for every promise
        for (unsigned long int index = 0; index < store->capacity; index ++){
            Promise *promise = store->promises[index];
            // if this slot is empty skip
            if (promise == NULL) continue;
            // update max access count (max threshold)
            if (promise->access_count > store->max_threshold){
                store->max_threshold = promise->access_count;
            }
            // if this cash needs to get cleaned
            if (threshold >= promise->access_count){
                // ofc free when no thread is working or waiting for the data
                //printf("waiting thread %d\nworking threads %d\n",promise->waiting_threads,
                //promise->working_threads);
                if (promise->waiting_threads == 0 && promise->working_threads == 0) {
                    // try to lock promise
                    if (pthread_mutex_trylock(&promise->lock) != 0)
                        continue;
                    // free the key
                    free(promise->key);
                    // free the data point associated with it
                    FreeDataPoint(promise->data);
                    // unlock the promise
                    pthread_mutex_unlock(&promise->lock);
                    // destroy the lock and flag
                    pthread_mutex_destroy(&promise->lock);
                    pthread_cond_destroy(&promise->ready);
                    // free promise
                    free(promise);
                    // update the pointer to null
                    store->promises[index] = NULL;
                    // update the count
                    store->count--;
                    // broadcast that we cleaned it , for any thread waiting for a free slot
                    pthread_cond_broadcast(&store->slot_available);
                }
            }
        }
        // unlock the store
        pthread_mutex_unlock(&store->lock);
        sleep(0.5);
    }
}