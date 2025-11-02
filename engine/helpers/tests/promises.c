#include "../helpers.h"


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
    Promise *p = get_create_promise(args->store, "test1");
    bool claimed = claim_work(p);
    if (p){
        if (claimed == true){
            printf("[+]thread %d got the work\n", args->id);
            sleep(10);
            Data *data = InitDataPoint();
            WriteData(data, STRING, "this is the work of 111111", DATA_NOT_OWNED);
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

    Promise **promises = calloc(100, sizeof(Promise));
    for (int x = 0; x < 100; x++){
        char *c = calloc(11 , sizeof(char));
        rand_str(c, 10);
        Promise *p = get_create_promise(args->store, c);
        //printf("created : %s\n", c);
        free(c);
        bool claimed = claim_work(p);
        promises[x] = p;
    }
    sleep(5);
    for (int x = 0; x < 100; x++){
        done_with_promise_data(promises[x]);
    }
    free(promises);

    Promise *p2 = get_create_promise(args->store, "test2");
    printf("added for test 2 \n");
    bool claimed2 = claim_work(p2);
    if (p2){
        if (claimed2 == true){
            printf("[+]thread %d got the work\n", args->id);
            sleep(10);
            Data *data = InitDataPoint();
            WriteData(data, STRING, "this is the work of 222222", DATA_NOT_OWNED);
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
    args->stop_flag = true;
    return NULL;
}


void *periodic_print(void *arg){
    thread_info *args = (thread_info *)arg;
    PromiseStore *store = args->store;
    int limit = 10;
    while (limit >= 0)
    {

        if (!store || args->stop_flag){
            break;
        }
        pthread_mutex_lock(&store->lock);
        printf("---------------------------------\n");
        printf("Store capacity : %lf\nStore threshold : %lf\n", 
            ((double)store->count/(double)store->capacity)*100, store->threshold);
        printf("store capacity : %ld\n max threshold = %lf, min threshold %lf\n", 
            store->capacity, store->max_threshold, store->min_threshold);
        printf("store count : %ld\n", store->count);
        printf("---------------------------------\n");
        pthread_mutex_unlock(&store->lock);
        limit --;
        sleep(1);
    }
   
    return NULL;
}

int main(){
    PromiseStore * store = InitPromiseStore();

    pthread_t *working_thread1 = calloc(1, sizeof(pthread_t));
    pthread_t *working_thread2 = calloc(1, sizeof(pthread_t));
    pthread_t *cleaner_thread_ = calloc(1, sizeof(pthread_t));
    pthread_t *monitor_thread_ = calloc(1, sizeof(pthread_t));

    thread_info *th = calloc(1, sizeof(thread_info));
    th->id = 1;
    th->store = store;
    // create threads
    pthread_create(working_thread1, NULL, foo, th);
    pthread_create(working_thread2, NULL, foo, th);
    pthread_create(cleaner_thread_, NULL,cleaner_thread, th);
    pthread_create(monitor_thread_, NULL,periodic_print, th);
    // join threads
    pthread_join(*cleaner_thread_, NULL);
    pthread_join(*working_thread1, NULL);
    pthread_join(*working_thread2, NULL);
    pthread_join(*monitor_thread_, NULL);
    // free store and promises when done
    free_promise_store(store);
    //free the mains allocations
    free(th);
    free(cleaner_thread_);
    free(working_thread1);
    free(working_thread2);
    free(monitor_thread_);
    return 0;
};