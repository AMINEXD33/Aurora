#include "../init/init.h"
#include "../../helpers/helpers.h"


int explore_btree_with_root(
    PromiseStore *store,
    Node *root,
    double threshold,
    unsigned long int index,
    double occupancy,
    int(do_something)(
        PromiseStore *store, 
        double threshold,
        unsigned long int index,
        double occupancy,
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

    for (unsigned long int node = 0; node < size; node++){
        Node *current = queue[node];
        if (do_something(store, threshold, index, occupancy, root, current) == -1){
            free(queue);
            return -1;
        }
    }
    
    free(queue);
    return 0;
}


int free_target_node_from_hmap(
    PromiseStore *store,  
    double threshold,
    unsigned long int index,
    double occupancy,
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
        if (pthread_mutex_trylock(&current->value.promise->lock) == 0){
        if ( (( (int)current->value.promise->waiting_threads <= 0 ) && 
            ( (int)current->value.promise->working_threads <= 0 )) ||
            occupancy >= 100
        ){
                printf("\t[XXXXXXXXXXXXXXXXXXX] freeing promise %s\n" ,current->value.promise->key);
                // pthread_mutex_unlock(&current->value.promise->lock);
                
                pthread_mutex_unlock(&current->value.promise->lock);
                // free promise
                Node *nd = free_node(
                    store->hashmap->node[index], 
                    current
                );
                // if root is NULL, well we're not allowed to assign NULL to
                // the table , we will create a node here
                if (!nd)
                    store->hashmap->node[index] = InitBtree();
                else
                    store->hashmap->node[index] = nd;

                // update the count
                store->count--;
                // broadcast that we cleaned it , for any thread waiting for a free slot
                pthread_cond_broadcast(&store->slot_available);
            }else{
                printf("\t\t[can't] can't free this fucker\n");
                printf("\t\t waiting = %d\n", current->value.promise->waiting_threads);
                printf("\t\t working = %d\n", current->value.promise->working_threads);
                pthread_mutex_unlock(&current->value.promise->lock);
                
            }
            
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
            pthread_mutex_unlock(&store->lock);
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
                index,
                occupancy,
                free_target_node_from_hmap);
        }
        // unlock the store
        pthread_mutex_unlock(&store->lock);
        printf("[+] cleaning round\n");
        if (threshold < 70){
            sleep(10);
        }else if (threshold > 70){
            sleep(1);
        }
    }
}