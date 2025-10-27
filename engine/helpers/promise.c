#include "./helpers.h"
#include <unistd.h>



/**
 * initiate Promise store
 */
PromiseStore *InitPromiseStore(){
    PromiseStore *store = malloc(sizeof(PromiseStore));
    if (!store){
        printf("can't allocate memory for the promise store\n");
        return NULL;
    }
    store->promises = calloc(1, sizeof(Promise*));
    if (!store->promises){
        printf("can't allocate mempry for promises pointers\n");
        free(store);
        return NULL;
    }

    store->count = 0;
    store->capacity = 1;
    pthread_mutex_init(&store->lock, NULL);
    pthread_cond_init(&store->slot_available, NULL);
    return store;
}

/**
 * find an empty slote to append a promise
 * this function is O(n)
 * ### return:
 *  `long int` : if slot found
 *  `-1`: if no slot is found 
 */
long int find_empty_slot(PromiseStore *store){
    for (long int index = 0; index < store->capacity; index++){
        // found an empty slot
        if (store->promises[index] == NULL){
            return index;
        }
    }
    return -1;
}

/**
 * create a new promise or return an already created one with the same
 * key , if no memory available for the new promise this function will
 * wait for the slot to be freed by the cleaner thread
 * #### NOTE!!!!!: you don't need to free the Data you've allocated
 * #### the cleaner thread will take care of it 
 * ### return :
 *   `Promise *`: when created or found
 *  
 */
Promise *get_create_promise(PromiseStore *store, const char *key){
    pthread_mutex_lock(&store->lock);
    // scan or wait for empty memory
    while (1) {
        // Always check for existing promise first
        for (unsigned long index = 0; index < store->capacity; index++){
            if (store->promises[index]){
                if (strcmp(store->promises[index]->key, key) == 0){
                    Promise *promise = store->promises[index];
                    
                    pthread_mutex_lock(&promise->lock);
                    promise->access_count += 1;
                    pthread_mutex_unlock(&promise->lock);
                    // unlock store here sinse we returning
                    pthread_mutex_unlock(&store->lock);
                    printf("[Thread %lu] Found existing: %s\n", 
                           pthread_self(), key);
                    return promise;
                }
            }
        }
        
        // Check if store is full
        if (store->count >= store->capacity){
            printf("[Thread %lu] Waiting for slot (count=%lu)...\n", 
                   pthread_self(), store->count);
            // wait for the memory to be freed
            pthread_cond_wait(&store->slot_available, &store->lock);
            printf("[Thread %lu] Woke up, rechecking...\n", pthread_self());
            // rescan
            continue;  // Re-check everything from the top
        }
        
        // We have space and key doesn't exist - safe to create
        break;
    }

    //init the promise
    Promise *promise = malloc(sizeof(Promise));
    promise->key = calloc(strlen(key)+1, sizeof(char));
    strcpy(promise->key, key);
    promise->status = PENDING;
    promise->data = NULL;
    promise->access_count = 0;
    promise->waiting_threads = 0;
    promise->working_threads = 0;
    // init the lock
    pthread_mutex_init(&promise->lock, NULL);
    // init the signal condition
    pthread_cond_init(&promise->ready, NULL);

    printf("create empty promise for key %s\n", promise->key);
    // this could happen , better safe
    // find a free slot
    long int free_slot = find_empty_slot(store);
    printf("[+] free slot = %ld\n", free_slot);
    // append 
    store->promises[free_slot] = promise;
    store->count++;
    pthread_mutex_unlock(&store->lock);
    return promise;
    
}

/**
 * claim the work of a promise,
 * this function locks the promise and checks
 * for the status of the promise , if promise is PENDING 
 * it claims the work and return true, if not then it returns false
 * ### return:
 *  `true`: work claimed
 *  `false`: work not claimed
 */
bool claim_work(Promise *promise){
    pthread_mutex_lock(&promise->lock);
    // check promise status
    if (promise->status == PENDING){
        promise->status = COMPUTING;
        // point out that a thread is working with this data
        promise->working_threads++;
        pthread_mutex_unlock(&promise->lock);
        printf("claimed work for key %s\n", promise->key);
        // we got the work
        return true;
    }
    printf("didn't claimed work for key %s\n", promise->key);
    pthread_mutex_unlock(&promise->lock);
    return false;
}

/**
 * publish the result of a promise with waiting threads
 */
void publish(Promise *promise, Data *result){
    if (!promise){
        printf("you need to pass a promise\n");
        return;
    }
    if (!result){
        printf("you need to pass a result\n");
        return;
    }
    printf("publishing key %s\n", promise->key);
    pthread_mutex_lock(&promise->lock);
    // append data
    promise->data = result;
    // set as ready
    promise->status = READY;
    pthread_cond_broadcast(&promise->ready);
    pthread_mutex_unlock(&promise->lock);
}

/**
 * wait for a promise to resolse
 */
Data *wait_for_result(Promise *promise){
    pthread_mutex_lock(&promise->lock);
    // point out that a thread is waiting for this data
    promise->waiting_threads++;
    // Wait until result is ready
    while (promise->status != READY){
        printf("waiting for key %s\n",promise->key);
        // wait for the promise to to be ready
        pthread_cond_wait(&promise->ready, &promise->lock);
    }
    // point out that the thread is no longer waiting for the data
    promise->waiting_threads--;
    // point out that the thread is working with data
    promise->working_threads++;
    // get data
    Data *result = promise->data;
    pthread_mutex_unlock(&promise->lock);
    return result;
}

/**
 * signal that a thread is done with what ever Promise and Data 
 * he aquired
 * #### NOTE!!!!!: you don't need to free the Data you've allocated
 * #### the cleaner thread will take care of it 
*/
void done_with_promise_data(Promise *promise){
    pthread_mutex_lock(&promise->lock);
    printf(">) done with the data promise\n");
    promise->working_threads--;
    printf("done and now working thread is %d and waiting thread is %d\n",
    promise->working_threads,
    promise->waiting_threads
    );
    pthread_mutex_unlock(&promise->lock);
}

typedef struct{
    PromiseStore *store;
    int id;
}thread_info;

void * foo(void *arg){
    thread_info *args = (thread_info *)arg;
    Promise *p = get_create_promise(args->store, "test1");
    bool claimed = claim_work(p);
    if (p){
        if (claimed == true){
            printf("[+]thread %d got the work\n", args->id);
            sleep(10);
            Data *data = malloc(sizeof(Data));
            WriteData(data, STRING, "12314", DATA_NOT_OWNED);
            publish(p, data);
            done_with_promise_data(p);
        }else if (claimed == false){
            printf("[+]thread %d is waiting for key %s\n", args->id, p->key);
            Data *data = wait_for_result(p);
            char *result = ReadDataStr(data);
            printf("[+]thread %d for the result of key %s = %s\n", 
                args->id, p->key, result);
            done_with_promise_data(p);
            
        }
    }
    Promise *p2 = get_create_promise(args->store, "test2");
    printf("added for test 2 \n");
    bool claimed2 = claim_work(p2);
    if (p2){
        if (claimed2 == true){
            printf("[+]thread %d got the work\n", args->id);
            sleep(10);
            Data *data = malloc(sizeof(Data));
            WriteData(data, STRING, "12314", DATA_NOT_OWNED);
            publish(p2, data);
            done_with_promise_data(p2);
        }else if (claimed2 == false){
            printf("[+]thread %d is waiting for key %s\n", args->id, p2->key);
            Data *data = wait_for_result(p2);
            char *result = ReadDataStr(data);
            printf("[+]thread %d for the result of key %s = %s\n", 
                args->id, p2->key, result);
            done_with_promise_data(p2);
            
        }
    }
    return NULL;
}

/**/
void *cleaner_thread(void *arg){
    thread_info *args = (thread_info *)arg;
    PromiseStore *store = args->store;
    
    while(true){
        // this will be replaiced with a conditional wait to preserve CPU BABYYYY
        if (store->count+10 > store->capacity){
            long int least_accessed = -1;
            pthread_mutex_lock(&store->lock);
            for (long int index = 0; index < store->capacity; index++){
                if (!store->promises[index]) continue;
                pthread_mutex_lock(&store->promises[index]->lock);
                if (least_accessed == -1){
                    least_accessed = index;
                }
                if (store->promises[index]->access_count < store->promises[least_accessed]->access_count){
                    least_accessed = index;
                }
                pthread_mutex_unlock(&store->promises[index]->lock);
            }
            if (least_accessed == -1) continue;
            Promise *promise = store->promises[least_accessed];
            if (!promise) continue;
            //printf("found the least accessed > %s\n", promise->key);
            printf("threads working = %d\nthreads waiting = %d\n", 
                promise->working_threads, 
                promise->waiting_threads);
            sleep(3);

            if (promise->waiting_threads == 0 && promise->working_threads == 0) {
                pthread_mutex_lock(&promise->lock);
                free(promise->key);
                FreeDataPoint(promise->data);
                pthread_mutex_unlock(&promise->lock);
                pthread_mutex_destroy(&promise->lock);
                pthread_cond_destroy(&promise->ready);
                free(promise);
                // update the pointer to null
                store->promises[least_accessed] = NULL;
                store->count--;
                printf("[x]freed a promise\n");
                printf("[x] count is = %d\n ", store->count);
                pthread_cond_broadcast(&store->slot_available);
            }
            pthread_mutex_unlock(&store->lock);
        }
        sleep(3);
    }

}
int main(){
    PromiseStore * store = InitPromiseStore();
    int thread_count = 5;
    pthread_t *threads = malloc(sizeof(pthread_t) * thread_count);
    for (int x = 0; x < thread_count; x++){
        thread_info *th = malloc(sizeof(thread_info));
        th->id = x;
        th->store = store;
        pthread_create(&threads[x], NULL, foo, th);
    }
    pthread_t *cleaner_thread_ = malloc(sizeof(pthread_t));
    thread_info *th = malloc(sizeof(thread_info));
    th->id = 60;
    th->store = store;
    pthread_create(cleaner_thread_, NULL,cleaner_thread, th);

    for (int x = 0; x < thread_count; x++){
        pthread_join(threads[x], NULL);
    }
    pthread_join(*cleaner_thread_, NULL);
    return 0;
}