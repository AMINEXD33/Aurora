#include "../init/init.h"
#include "../../helpers/helpers.h"


int explore_btree_with_root(
    PromiseStore *store,
    Node *root,
    double threshold,
    int(do_something)(
        PromiseStore *store, 
        Node *root,
        double threshold,
        Node *current_node)
) {
    if (!root) return -1;
    
    // a simple array as queue
    size_t capacity = 256 * 4;
    size_t size = 0;
    size_t front = 0;
    Node **queue = malloc(capacity * sizeof(Node*));
    
    if (!queue) {
        printf("[ERROR] Can't allocate queue\n");
        return -1;
    }
    
    queue[size++] = root;
    
    while (front < size) {
        Node *current = queue[front++];
        
        // add children to queue before freeing
        if (current->left) {
            if (size >= capacity) {
                capacity *= 2;
                queue = realloc(queue, capacity * sizeof(Node*));
            }
            queue[size++] = current->left;
        }
        
        if (current->right) {
            if (size >= capacity) {
                capacity *= 2;
                queue = realloc(queue, capacity * sizeof(Node*));
            }
            queue[size++] = current->right;
        }
        
        // do something with current
        if (do_something(store , root,threshold, current) == -1){
            free(queue);
            printf("[x] something wrong in the do something func!\n");
            return -1;
        }
    }
    free(queue);
    return 0;
}

int free_target_node_from_hmap(
    PromiseStore *store,  
    Node *root,
    double threshold, 
    Node *current)
    {
    // ignore the nodes that are null or the that don't contain a promise 
    if (!current || !current->value.promise) return 0;
    // update max access count (max threshold)
    if (current->value.promise->access_count > store->max_threshold){
        store->max_threshold = current->value.promise->access_count;
    }
    // if this cash needs to get cleaned
    if (threshold >= current->value.promise->access_count){
        // ofc free when no thread is working or waiting for the data
        //printf("waiting thread %d\nworking threads %d\n",promise->waiting_threads,
        //promise->working_threads);
        if (current->value.promise->waiting_threads == 0 && current->value.promise->working_threads == 0) {
            // try to lock promise
            if (pthread_mutex_trylock(&current->value.promise->lock) != 0)
                return 0;
            
            printf("\t[XXXXXXXXXXXXXXXXXXX] freeing promise %s" ,current->value.promise->key);
            // free promise
            free_promise(current->value.promise);
            // update the count
            store->count--;
            // broadcast that we cleaned it , for any thread waiting for a free slot
            pthread_cond_broadcast(&store->slot_available);
        }else{
            printf("\t[x] threads still working with this\n");
            printf("waiting = %d , working = %d\n", current->value.promise->waiting_threads, current->value.promise->working_threads);
        }
    }else{
        printf("\t[x] promise didn't catche threshold %lf  , promise access count = %ld \n", threshold, current->value.promise->access_count);
    }
    return 0;
}
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
            sleep(1);
            continue;
        }
        // lock the store
        pthread_mutex_lock(&store->lock);
        // get the new threshold
        double threshold = update_store_threshold(store);
        // for every promise
        for (unsigned long int index = 0; index < store->hashmap->size; index ++){
            Node *root = store->hashmap->node[index];
            explore_btree_with_root(
                store, 
                root,
                threshold, 
                free_target_node_from_hmap);
        }
        // unlock the store
        pthread_mutex_unlock(&store->lock);
        printf("[+] cleaning round\n");
        sleep(0.3);
    }
}