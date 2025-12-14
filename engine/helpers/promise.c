#include "./helpers.h"


/**
 * initiate Promise store
 */
PromiseStore *InitPromiseStore(
    unsigned int size,
    double threshold,
    double min_threshold,
    double max_threshold
){
    PromiseStore *store = calloc(1, sizeof(PromiseStore));
    if (!store){
        printf("can't allocate memory for the promise store\n");
        return NULL;
    }
    if (size < 1){
        printf("promise store size can't be less than 1\n");
        return NULL;
    }
    if (threshold < 0 || min_threshold < 0 || max_threshold < 0){
        printf("threashold/ min_threshold/ max_threshold can't be negative\n");
        return NULL;
    }
    if (min_threshold > max_threshold){
        printf("min threshold can't be sup than max threshold\n");
        return NULL;
    }
    store->hashmap = InitHashMap(100000);
    if (!store->hashmap){
        printf("can't allocate memory for promise hashmap\n");
        free(store);
        return NULL;
    }
    store->count = 0;
    store->capacity = size;
    store->smoothing = 0.1;
    store->ema_occupancy = 0.0;
    store->prev_ema = 0.0;
    store->threshold = threshold;
    store->min_threshold = min_threshold;
    store->max_threshold = max_threshold;
    // initiate the cleanup with time of creation
    pthread_mutex_init(&store->lock, NULL);
    pthread_cond_init(&store->slot_available, NULL);
    return store;
}

void free_promise_store(PromiseStore *store){
    printf("[*]freeing stuff\n");
    pthread_mutex_lock(&store->lock);
    free_hashmap_and_data(store->hashmap);
    pthread_mutex_unlock(&store->lock);
    pthread_mutex_destroy(&store->lock);
    pthread_cond_destroy(&store->slot_available);
    free(store);
}

/**
 * THIS SUCKS , AND IT'S TEMP , we need something that is O(1) best case
 * and O(log n) worse
 */
Promise *get_promise(PromiseStore *store, char *key){
Node *node = hash_search(store->hashmap, key);
    if (!node || node->type != PROMISE || !node->value.promise) {
        // Consolidated search/null checks
        if (!node) printf("[x] can't find promise with key %s\n", key);
        else if (node->type != PROMISE) printf("[x] cache found but not of type Promise, key: %s\n", key);
        return NULL;
    }

    Promise *p = node->value.promise;
    Promise *pr = NULL;

    // Use trylock to avoid blocking, honoring the user's intent:
    if (pthread_mutex_trylock(&p->lock) == 0) { // Success returns 0
        // Lock acquired successfully!
        // Increment access count here (protected by lock)
        p->access_count += 1;
        
        // Deep copy the promise data
        pr = deep_copy_Promise(p);
        
        pthread_mutex_unlock(&p->lock); // Unlock when done with access
    } else {
        // Lock failed (another thread has the lock)
        printf("[xXXXX] couldn't lock it to deep copy it. Skipping this read.\n");
        return NULL;
    }

    if (!pr){
        printf("[x] the node is promise but the deep copy failed\n");
        return NULL;
    }
    return pr;
}
/**
 * Initiate an empty promise
 * ### args:
 *  `key`: the key of the promise
 * ### return
 *  `NULL`: allocation failed
 *  `Promise *`: pointer to the promise
 */
Promise *InitPromise(char *key){
    if (!key)
        return NULL;
    Promise *promise = malloc(sizeof(Promise));
    if (!promise){
        return NULL;
    }
    promise->key = strdup(key);
    if (!promise->key){
        free(promise);
        return NULL;
    }
    promise->status = PENDING;
    promise->datatype.array = NULL;
    promise->datatype.data = NULL;
    promise->access_count = 10;
    promise->waiting_threads = 0;
    promise->working_threads = 0;
    promise->type = NOTHING;
    promise->marked_for_deletion = false;
        // init the lock
    pthread_mutex_init(&promise->lock, NULL);
    // init the signal condition
    pthread_cond_init(&promise->ready, NULL);
    return promise;
}

void free_promise(Promise *promise){
    if (!promise)
        return;
    
    switch (promise->type)
    {
        case ARRAY:
            free_array(promise->datatype.array);
            break;
        case DATA:
            FreeDataPoint(promise->datatype.data);
            break;
    }
    free(promise->key);
    free(promise);
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
Promise *get_create_promise(PromiseStore *store, char *key){
    if (!store){
        printf("store is not passed\n");
        return NULL;
    }
    if (!key){
        printf("no key is provided\n");
        return NULL;
    }
    
    // scan or wait for empty memory
    while (1) {
        // Always check for existing promise first
        Node *promise = hash_search(store->hashmap, key);
        if (promise && promise->type == PROMISE && promise->value.promise){
            // this cashe is accessed one more time
            if (pthread_mutex_trylock(&promise->value.promise->lock) == 0){
                promise->value.promise->access_count += 1;
                pthread_mutex_unlock(&promise->value.promise->lock);
                // unlock store here sinse we returning
                pthread_mutex_unlock(&store->lock);
                return promise->value.promise;
            }
        }
        // Check if store is full
        if (store->count >= store->capacity){
            //printf("[Thread %lu] Waiting for slot (count=%lu)...\n", 
                   //pthread_self(), store->count);
            // wait for the memory to be freed
            //<! trying with out> pthread_cond_wait(&store->slot_available, &store->lock);
            return NULL;
            //printf("[Thread %lu] Woke up, rechecking...\n", pthread_self());
            // rescan
            
            //continue;  // Re-check everything from the top
        }
        
        // We have space and key doesn't exist, safe to create
        break;
    }
    pthread_mutex_lock(&store->lock);
    //init the promise
    Promise *promise = InitPromise(key);

    //printf("create empty promise for key %s\n", promise->key);
    // this could happen , better safe
    // append 
    hash_push_Promise(store->hashmap, promise);
    // pthread_mutex_lock(&promise->lock);
    store->count++;
    // pthread_mutex_unlock(&promise->lock);
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
    if (!promise)
        return false;
    if (pthread_mutex_trylock(&promise->lock) == 0){
        // check promise status
        if (promise->status == PENDING){
            promise->status = COMPUTING;
            // point out that a thread is working with this data
            //promise->working_threads++;
            pthread_mutex_unlock(&promise->lock);
            //printf("claimed work for key %s\n", promise->key);
            // we got the work
            return true;
        }
        //printf("didn't claimed work for key %s\n", promise->key);
        pthread_mutex_unlock(&promise->lock);
        return false;
        }
    // can't get lock fail
    return false;
}

/**
 * publish the result of a promise with waiting threads
 */
void publishData(Promise *promise, Data *result){
    if (!promise){
        printf("you need to pass a promise\n");
        return;
    }
    if (!result){
        printf("you need to pass a result\n");
        return;
    }
    //printf("publishing key %s\n", promise->key);
    if (pthread_mutex_trylock(&promise->lock) == 0){
        // append data
        promise->datatype.data = result;
        // set as ready
        promise->status = READY;
        promise->type = DATA;
        pthread_cond_broadcast(&promise->ready);
        pthread_mutex_unlock(&promise->lock);
    }
}

/**
 * publish the result of a promise with waiting threads
 */
void publishArray(Promise *promise, Array *result){
    if (!promise){
        printf("you need to pass a promise\n");
        return;
    }
    if (!result){
        printf("you need to pass a result\n");
        return;
    }
    //printf("publishing key %s\n", promise->key);
    if (pthread_mutex_trylock(&promise->lock) == 0){
        // append data
        promise->datatype.array = result;
        // set as ready
        promise->status = READY;
        promise->type = ARRAY;
        pthread_cond_broadcast(&promise->ready);
        pthread_mutex_unlock(&promise->lock);
    }
}
/**
 * wait for a promise of datatype Data to resolse
 */
Data *wait_for_result_data(Promise *promise){
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
    Data *result = promise->datatype.data;
    pthread_mutex_unlock(&promise->lock);
    return result;
}

/**
 * wait for a promise of datatype Array to resolse
 */
Array *wait_for_result_array(Promise *promise){
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
    Array *result = promise->datatype.array;
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
    if (!promise)
        return;
    pthread_mutex_lock(&promise->lock);
    //printf(">) done with the data promise\n");
    promise->working_threads--;

    pthread_mutex_unlock(&promise->lock);
}




/**
 * calculate the ema of the (count/capacity) , and based on the trend 
 * and the pressure make an adjustment to the threshold
 * ### return:
 *  `double`
 */
double update_store_threshold(PromiseStore *store){
    // calc accupancy 
    double occupancy = ((double)store->count / (double)store->capacity) * 100;
    //printf("\t[x] occupancy = %lf\n", occupancy);

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
    // calc the new threshold
    store->threshold = store->threshold * adjustment;
    // make sure it respects min < threshold < max
    if (store->threshold > store->max_threshold){
        store->threshold = store->max_threshold;
    }
    if (store->threshold < store->min_threshold){
        store->threshold = store->min_threshold;
    }
    // store prev ema
    store->prev_ema = store->ema_occupancy;

    return store->threshold;
}