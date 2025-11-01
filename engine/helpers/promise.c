#include "./helpers.h"


struct timeval *make_clock() {
    struct timeval *t = malloc(sizeof(struct timeval));
    if (!t){
        return NULL;
    }
    return t;
}

double calc_delta(struct timeval *start,struct timeval *end){
    
    double delta = (end->tv_sec - start->tv_sec) + 
                   (end->tv_usec - start->tv_usec) * 1e-6;
    return delta;
}

/**
 * initiate Promise store
 */
PromiseStore *InitPromiseStore(){
    PromiseStore *store = malloc(sizeof(PromiseStore));
    if (!store){
        printf("can't allocate memory for the promise store\n");
        return NULL;
    }
    store->promises = calloc(10000, sizeof(Promise*));
    if (!store->promises){
        printf("can't allocate memory for promises pointers\n");
        free(store);
        return NULL;
    }
    store->last_cleanup_start = make_clock();
    if (!store->last_cleanup_start){
        printf("can't allocate memory for last cleanup start clock\n");
        free(store->promises);
        free(store);
    }
    store->count = 0;
    store->capacity = 10000;
    store->smoothing = 0.1;
    store->ema_occupancy = 0.0;
    store->prev_ema = 0.0;
    store->threshold = 10;
    store->min_threshold = 1;
    store->max_threshold = 100;
    store->max_recorded = 0.0;
    // initiate the cleanup with time of creation
    gettimeofday(store->last_cleanup_start, NULL);
    pthread_mutex_init(&store->lock, NULL);
    pthread_cond_init(&store->slot_available, NULL);
    return store;
}
void free_promise_store(){
    
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
        for (long int index = 0; index < store->capacity; index++){
            if (store->promises[index]){
                if (strcmp(store->promises[index]->key, key) == 0){
                    Promise *promise = store->promises[index];
                    
                    pthread_mutex_lock(&promise->lock);
                    // this cashe is accessed one more time
                    promise->access_count += 1;
                    pthread_mutex_unlock(&promise->lock);
                    // unlock store here sinse we returning
                    pthread_mutex_unlock(&store->lock);
                    //printf("[Thread %lu] Found existing: %s\n", 
                           //pthread_self(), key);
                    return promise;
                }
            }
        }
        
        // Check if store is full
        if (store->count >= store->capacity){
            //printf("[Thread %lu] Waiting for slot (count=%lu)...\n", 
                   //pthread_self(), store->count);
            // wait for the memory to be freed
            pthread_cond_wait(&store->slot_available, &store->lock);
            //printf("[Thread %lu] Woke up, rechecking...\n", pthread_self());
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
    promise->access_count = 100;
    promise->waiting_threads = 0;
    promise->working_threads = 0;
    // init the lock
    pthread_mutex_init(&promise->lock, NULL);
    // init the signal condition
    pthread_cond_init(&promise->ready, NULL);

    //printf("create empty promise for key %s\n", promise->key);
    // this could happen , better safe
    // find a free slot
    long int free_slot = find_empty_slot(store);
    //printf("[+] free slot = %ld\n", free_slot);
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
        //printf("claimed work for key %s\n", promise->key);
        // we got the work
        return true;
    }
    //printf("didn't claimed work for key %s\n", promise->key);
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
    //printf("publishing key %s\n", promise->key);
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
        //printf("waiting for key %s\n",promise->key);
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
    //printf(">) done with the data promise\n");
    promise->working_threads--;

    pthread_mutex_unlock(&promise->lock);
}

typedef struct{
    PromiseStore *store;
    int id;
}thread_info;
void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}
void * foo(void *arg){
    thread_info *args = (thread_info *)arg;
    // Promise *p = get_create_promise(args->store, "test1");
    // bool claimed = claim_work(p);
    // if (p){
    //     if (claimed == true){
    //         printf("[+]thread %d got the work\n", args->id);
    //         sleep(10);
    //         Data *data = malloc(sizeof(Data));
    //         WriteData(data, STRING, "this is the work of 111111", DATA_NOT_OWNED);
    //         publish(p, data);
    //         done_with_promise_data(p);
    //     }else if (claimed == false){
    //         printf("[+]thread %d is waiting for key %s\n", args->id, p->key);
    //         Data *data = wait_for_result(p);
    //         char *result = ReadDataStr(data);
    //         printf("[+]thread %d for the result of key %s = %s\n", 
    //             args->id, p->key, result);
    //         done_with_promise_data(p);
            
    //     }
    // }

    Promise **promises = malloc(sizeof(Promise) * 100);
    for (int x = 0; x < 300; x++){
        char *c = malloc(sizeof(char) * 10);
        rand_str(c, 10);
        Promise *p = get_create_promise(args->store, c);
        //printf("created : %s\n", c);
        bool claimed = claim_work(p);
        promises[x] = p;
    }
    sleep(5);
    for (int x = 0; x < 300; x++){
        done_with_promise_data(promises[x]);
    }
    // Promise *p2 = get_create_promise(args->store, "test2");
    // printf("added for test 2 \n");
    // bool claimed2 = claim_work(p2);
    // if (p2){
    //     if (claimed2 == true){
    //         printf("[+]thread %d got the work\n", args->id);
    //         sleep(10);
    //         Data *data = malloc(sizeof(Data));
    //         WriteData(data, STRING, "this is the work of 222222", DATA_NOT_OWNED);
    //         publish(p2, data);
    //         done_with_promise_data(p2);
    //     }else if (claimed2 == false){
    //         printf("[+]thread %d is waiting for key %s\n", args->id, p2->key);
    //         Data *data = wait_for_result(p2);
    //         char *result = ReadDataStr(data);
    //         printf("[+]thread %d for the result of key %s = %s\n", 
    //             args->id, p2->key, result);
    //         done_with_promise_data(p2);
            
    //     }
    // }
    return NULL;
}


double update_store_threshold(PromiseStore *store){
    // calc accupancy 
    double occupancy = ((double)store->count / (double)store->capacity) * 100;

    // update ema
    store->ema_occupancy =store->smoothing * occupancy + (1 - store->smoothing) * store->prev_ema;
    // calculate trend
    double trend = store->ema_occupancy - store->prev_ema;
    // calc adjustments
    double adjustment = 1;
    if (trend > 0){
        // pressure going up
        if (store->ema_occupancy > 70){
            // critical , go up fast
            adjustment = 1 + 0.2 * tanh(trend / 10);
        }else{
            // go up slowly
            adjustment = 1 + 0.1 * tanh(trend / 10);
        }
    }
    else if(trend < 0){
        // pressure going down
        adjustment = 0.9;
    }
    else{
        // same level
        adjustment = 1;
    }

    // occupancy multiplier
    if (store->ema_occupancy > 95){
        // be aggressive
        adjustment *= 1.3;
    }
    else if (store->ema_occupancy > 85){
        // little bit aggressive
        adjustment *= 1.2;
    }
    else if (store->ema_occupancy < 30){
        // relax
        adjustment *= 1.0;
    }

    store->threshold = store->threshold * adjustment;
    if (store->threshold > store->max_threshold){
        store->threshold = store->max_threshold;
    }
    if (store->threshold < store->min_threshold){
        store->threshold = store->min_threshold;
    }
    store->prev_ema = store->ema_occupancy;

    if (store->threshold > store->max_recorded){
        store->max_recorded = store->threshold;
    }

    return store->threshold;
}



/**/
void *cleaner_thread(void *arg){
    thread_info *args = (thread_info *)arg;
    PromiseStore *store = args->store;
    sleep(1);
    // see delta of last clean
    struct timeval *end = make_clock();
    int cleaned = 0;
    while(true){
        double occupancy = ((double)store->count / (double)store->capacity) * 100;
        if (occupancy < 50){
            sleep(0.1);
        }
        // this will be replaiced with a conditional wait to preserve CPU BABYYYY
        pthread_mutex_lock(&store->lock);
        double threshold = update_store_threshold(store);
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
                    pthread_mutex_lock(&promise->lock);
                    free(promise->key);
                    FreeDataPoint(promise->data);
                    pthread_mutex_unlock(&promise->lock);
                    pthread_mutex_destroy(&promise->lock);
                    pthread_cond_destroy(&promise->ready);
                    free(promise);
                    // update the pointer to null
                    store->promises[index] = NULL;
                    store->count--;
                    //printf("[x]freed a promise\n");
                    //printf("[x] count is = %ld\n ", store->count);
                    // broadcast that we cleaned it , for any thread waiting for a free slot
                    pthread_cond_broadcast(&store->slot_available);
                    cleaned++;
                }
            }
        }
        pthread_mutex_unlock(&store->lock);
        //printf("_____cleaned = %d\n", cleaned);
        sleep(0.5);
    }
}

void *periodic_print(void *arg){
    thread_info *args = (thread_info *)arg;
    PromiseStore *store = args->store;

    while (true)
    {
        pthread_mutex_lock(&store->lock);
        printf("---------------------------------\n");
        printf("Store capacity : %lf\nStore threshold : %lf\n", 
            ((double)store->count/(double)store->capacity)*100, store->threshold);
        printf("store capacity : %ld\n max threshold = %lf, min threshold %lf\n", 
            store->capacity, store->max_threshold, store->min_threshold);
        printf("store count : %ld\n", store->count);
        printf("max threshold : %lf\n", store->max_recorded);
        printf("---------------------------------\n");
        pthread_mutex_unlock(&store->lock);
        sleep(1);
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

    pthread_t *loging_thread = malloc(sizeof(pthread_t));
    pthread_create(loging_thread, NULL, periodic_print, th);


    for (int x = 0; x < thread_count; x++){
        pthread_join(threads[x], NULL);
    }
    pthread_join(*cleaner_thread_, NULL);
    pthread_join(*loging_thread, NULL);
    return 0;
};