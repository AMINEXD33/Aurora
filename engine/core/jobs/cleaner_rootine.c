#include "../init/init.h"
#include "../../helpers/helpers.h"


int explore_btree_with_root(
    PromiseStore *store,
    Node *root,
    double threshold,
    int(do_something)(
        PromiseStore *store, 
        double threshold,
        Node *root,
        Node *current_node)
) {
    if (!root) return -1;
    
    size_t capacity = 256 * 4;
    size_t size = 0;
    size_t front = 0;
    Node **queue = malloc(capacity * sizeof(Node*));
    
    if (!queue) return -1;
    
    queue[size++] = root;
    
    // Process while building the queue
    while (front < size) {
        Node *current = queue[front++];
        
        // Process THIS node before adding children
        if (do_something(store, threshold, root, current) == -1){
            free(queue);
            return -1;
        }
        
        // NOW add children (if current wasn't deleted)
        // Check if current is still valid first!
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
    }
    
    free(queue);
    return 0;
}


int free_target_node_from_hmap(
    PromiseStore *store,  
    double threshold,
    Node *root,
    Node *current)
    {
    // ignore the nodes that are null or the that don't contain a promise 
    if (!current || current->type != PROMISE || !current->value.promise) {
        return 0;
    }
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
            
            printf("\t[XXXXXXXXXXXXXXXXXXX] freeing promise %s\n" ,current->value.promise->key);
            unsigned long bucket = current->hashed_key % store->hashmap->size;
            // pthread_mutex_unlock(&current->value.promise->lock);
            
            pthread_mutex_unlock(&current->value.promise->lock);
            // free promise
            store->hashmap->node[bucket] = free_node(
                store->hashmap->node[bucket], 
                current
            );
            // update the count
            store->count--;
            // broadcast that we cleaned it , for any thread waiting for a free slot
            pthread_cond_broadcast(&store->slot_available);
         }else{
             printf("\t\t[can't] can't free this fucker\n");
             printf("\t\t waiting = %d\n", current->value.promise->waiting_threads);
             printf("\t\t working = %d\n", current->value.promise->working_threads);
         }
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
        // get the new threshold
        double threshold = update_store_threshold(store);
        if (occupancy < 50){
            sleep(3);
            continue;
        }
        // lock the store
        pthread_mutex_lock(&store->lock);
        printf("\t\t[threshold] %lf\n", threshold);
        // for every promise
        for (unsigned long int index = 0; index < store->hashmap->size; index ++){
            if (!store->hashmap->node[index]) continue;
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
        sleep(3);
    }
}