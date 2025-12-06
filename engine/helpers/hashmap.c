#include "./helpers.h"



/**
 * initialize a hashmap with some size
 * ### args:
 *  `initial_size`: the initial size of the hashmap
 * ### return:
 *  `return`: 
 *     `Hashmap *`: pointer to the initialized hashmap
 *     `NULL`: can't allocate the hashmap
 */
Hashmap* InitHashMap(unsigned long int initial_size){
    if (initial_size <= 0)
        initial_size = 10;
    
    Hashmap *hashmap = calloc(1, sizeof(Hashmap));
    if (!hashmap){
        return NULL;
    }
    hashmap->node = malloc(sizeof(Node *) * initial_size);

    if (!hashmap->node){
        free(hashmap);
        return NULL;
    }
    // initialize all nodes
    unsigned long int x = 0;
    for (x = 0; x < initial_size; x++){
        hashmap->node[x] = InitBtree();
        if (!hashmap->node[x]){
            break;
        }
    }
    // incase of an error free the allocated nodes
    if (x != initial_size){
        x--;// do not include the one we failed to allocate
        for(x; x > 0; x--){
            free_tree_bfs(hashmap->node[x]);
        }
        free(hashmap);
        return NULL;
    }
    hashmap->size = initial_size;
    hashmap->inserts = 0;
    return hashmap;
}

/**
 * free a hashmap
 * ### args:
 *  `hashmap`: pointer to the hashmap 
 */
void free_hashmap_and_data(Hashmap *hashmap){

    if (!hashmap)
        return;
    for (unsigned long int x = 0; x < hashmap->size; x++){
        free_tree_bfs(hashmap->node[x]);
    }
    free(hashmap->node);
    free(hashmap);
}


Node *hash_search(Hashmap *hashmap, char* key){
    if (!hashmap || !key)
        return NULL;
    XXH64_hash_t key_hash = XXH64(key, strlen(key), 0);
    size_t index = key_hash % (hashmap->size - 1);

    Node *target = get_Node_from_tree(key, hashmap->node[index]);
    if (!target){
        return NULL;
    }
    return target;
}

int hash_push_value(Hashmap *hashmap, char *key, complex_structures type, void *value){
    if (!hashmap || !key)
        return -1;
    
    XXH64_hash_t key_hash = XXH64(key, strlen(key), 0);
    size_t index = key_hash % (hashmap->size - 1);
    int inserted = 0;
    switch (type)
    {
        case DATA:
            inserted = push_Data_to_tree(value, hashmap->node[index]);
            break;
        case ARRAY:
            inserted = push_Array_to_tree(value, hashmap->node[index]);
            break;
        case PROMISE:
            inserted = push_Promise_to_tree(value, hashmap->node[index]);
            break;
        default:
            return -1;
    }
    // track insertions
    if (inserted == 0){
        hashmap->inserts++;
        return 0;
    }
    return -1;
}

int hash_push_Data(Hashmap *hashmap, Data *value){
    if (!value || !value->key)
        return -1;
    if(hash_push_value(hashmap, value->key, DATA, value) != -1){
        return 0;
    }
    return -1;
}
int hash_push_Array(Hashmap *hashmap, Array *value){
    if (!value || !value->key)
        return -1;
    if (hash_push_value(hashmap, value->key, ARRAY, value) != -1){
        return 0;
    }
    return -1;
}

int hash_push_Promise(Hashmap *hashmap, Promise *value){
    if (!value || !value->key)
        return -1;
    if (hash_push_value(hashmap, value->key, PROMISE, value) != -1){
        return 0;
    }
    return -1;
}

Data *deep_copy_Data(Data *data){
    if (!data)
        return NULL;
    Data *dt = InitDataPoint(data->key);
    dt->type = data->type;
    switch (data->type)
    {
        case STRING:
            WriteDataString(dt, data->value.string_val);
            break;
        case BOOLEAN:
            dt->value.bool_val = data->value.bool_val;
            break;
        case FLOAT:
            dt->value.float_val = data->value.float_val;
            break;
        case INT:
            dt->value.int_val = data->value.int_val;
            break;
        case DOUBLE:
            dt->value.double_val = data->value.double_val;
            break;
        case LONG:
            dt->value.long_val = data->value.long_val;
            break;
        default:
            break;
    }
    return dt;
}
Array *deep_copy_Array(Array *array){
    Array *arr = malloc(sizeof(Array));
    if (!arr){
        return NULL;
    }
    arr->index = array->index;
    arr->key = strdup(array->key);
    arr->size = array->size;
    arr->array = malloc(sizeof(Data *) * arr->size);
    if (!arr->array){
        free(arr);
        return NULL;
    }
    unsigned int x = 0;
    for (x ; x < array->index; x++){
        arr->array[x] = deep_copy_Data(array->array[x]);
    }
    for (x; x < array->size; x++){
        arr->array[x] = NULL;
    }
    return arr;
}

Promise *deep_copy_Promise( Promise *promise){
    if (!promise){
        return NULL;
    }
    // the key is copied , so no need to do it
    Promise *pr = InitPromise(promise->key);
    // only copy the other stuff , and not the locks ! , don't touch the locks!
    pr->status = promise->status;
    pr->type = promise->type;
    pr->datatype = promise->datatype;
    pr->waiting_threads = promise->waiting_threads;
    pr->working_threads = promise->working_threads;
    pr->access_count = promise->access_count;
    switch (pr->type)
    {
        case DATA:
            pr->datatype.data = deep_copy_Data(promise->datatype.data);
            break;
        case ARRAY:
            pr->datatype.array = deep_copy_Array(promise->datatype.array);
            break;
        default:
            break;
    }
    return pr;
}

int clone_node_to_new_hmap(Hashmap *new_hashmap, Node *some_node){
    // new position
    unsigned long int index = some_node->hashed_key % new_hashmap->size;
    // deep copy the data and push to the new one
    int inserted = 0;
    switch (some_node->type)
    {
        case DATA:
            Data *dt = deep_copy_Data(some_node->value.data);
            inserted =  push_Data_to_tree(
                dt,
                new_hashmap->node[index]
            );
            break;
        case ARRAY:
            Array *arr = deep_copy_Array(some_node->value.array);
            inserted = push_Array_to_tree(
                arr,
                new_hashmap->node[index]
            );
            break;
        case PROMISE:
            Promise *pr = deep_copy_Promise(some_node->value.promise);
            inserted =  push_Promise_to_tree(
                pr,
                new_hashmap->node[index]
            );
            break;
    }
    if (inserted == 0){
        new_hashmap->inserts++;
        return 0;
    }
    return -1;
}
/**
 * Resize a hashmap. This function uses O(n + k + d) space,
 * where n is the size of the old table, k is the new size,
 * and d represents the temporary copy of the data.
 * This approach allows the hashmap to be rebuilt while the
 * old one is still accessible, with no waiting time.
 */
Hashmap *resize_hashmap(Hashmap *old_hashmap){
    if (!old_hashmap){
        printf("[x] there is no old hashmap\n");
        return NULL;
    }
    Hashmap *new_hashmap  = InitHashMap(old_hashmap->size * 2);
    // for all buckets
    for (unsigned long int index = 0; index < old_hashmap->size; index++){
        // for all nodes in the binary tree

        // skip any node that hold nothing
        if (old_hashmap->node[index]->type == NOTHING){
            continue;
        }
        int insert_status = explore_btree(
            new_hashmap, 
            old_hashmap->node[index],
            clone_node_to_new_hmap
        );
        if (insert_status == -1){
            // something went wrong while inserting
            free_hashmap_and_data(new_hashmap);
            printf("[x]something went wrong while inserting\n");
            return NULL;
        }
    }
    return new_hashmap;
}
// int main (){
//     Array *arr1 = InitDataPointArray("array1");
//     for (int x = 0; x < 100; x++){
//         Data *tmp = InitDataPoint("xff1");
//         if (x % 2){
//             WriteDataString(tmp, "test12");
//         }else{
//             WriteDataFloat(tmp, 1241.1152);
//         }
//         append_datapoint(arr1, tmp);
//     }
//     Data *data = InitDataPoint("asda");
//     WriteDataFloat(data, 123.124124);
//     Promise *pr1 = InitPromise("pr1");
//     pr1->type = DATA;
//     pr1->datatype.data = data;
//     Promise *pr2 = InitPromise("pr2");
//     pr2->type = ARRAY;
//     pr2->datatype.array = arr1;

//     Promise *pr1_cpy = deep_copy_Promise(pr1);
//     Promise *pr2_cpy = deep_copy_Promise(pr2);
    
//     free_promise(pr1);
//     free_promise(pr2);

//     // printArray(pr2_cpy->datatype.array);
//     // printDataPoint(pr1_cpy->datatype.data, "\n");


//     free_promise(pr1_cpy);
//     free_promise(pr2_cpy);
    

// int main (){
//     srand(time(NULL));
//     const int NUM_NODES = 10000;
//     Hashmap *hashmap = InitHashMap(100000);
//     printf("[*] Starting stress test with %d nodes...\n", NUM_NODES * 3);

//     // store keys and data stuff
//     char **data_keys = malloc(NUM_NODES * sizeof(char *));
//     char **array_keys = malloc(NUM_NODES * sizeof(char *));
//     char **promise_keys = malloc(NUM_NODES * sizeof(char *));

//     double start = 0.0;

//     for (int i = 0 ; i < NUM_NODES; i++){
//         data_keys[i] = malloc(128);
//         array_keys[i] = malloc(128);
//         promise_keys[i] = malloc(128);
//         rand_str(data_keys[i], 127);
//         rand_str(array_keys[i], 127);
//         rand_str(promise_keys[i], 127);
//         // gen data
//         Data *dt = InitDataPoint(data_keys[i]);
//         WriteDataInt(dt, 1234);
//         // gen array of data
//         Array *arr = InitDataPointArray(array_keys[i]);
//         // put in a 10 elements with some stuff in them
//         for (int tmp = 0; tmp < 10; tmp++){
//             Data *dt_tmp = InitDataPoint(NULL);

//             if (tmp % 2){
//                 char *tmp_char = calloc(20, sizeof(char));
//                 rand_str(tmp_char, 19);
//                 WriteDataString(dt_tmp, tmp_char);
//                 free(tmp_char);
//             }else{
//                 WriteDataInt(dt_tmp, rand());
//             }
//             append_datapoint(arr, dt_tmp);
            
//         }
//         // gen promise
//         Promise *pr = InitPromise(promise_keys[i]);
//         // push stuff to the tree
//         clock_t sub_start = clock();// only record the inser to the tree
//         if (hash_push_Data(hashmap, dt) == -1){
//             printf("[x] can't push data !\n");
//         }
//         if (hash_push_Array(hashmap, arr)==-1){
//             printf("[x] can't push array !\n");
//         };
//         if (hash_push_Promise(hashmap, pr) == -1){
//             printf("[x] can't push promise !\n");
//         }
//         clock_t sub_end = clock();
//         start += ((double)(sub_end - sub_start)) / CLOCKS_PER_SEC;
//     }
//     clock_t end = clock();
//     double insert_time = ((double)(end - start)) / CLOCKS_PER_SEC;
//     printf("[✓] Inserted %d nodes in %.3f seconds (%.0f nodes/sec)\n", 
//            NUM_NODES*3, insert_time, NUM_NODES*3 / insert_time);

//     /** PHASE 2: SEARCH ALL NODES */
//     printf("\n[PHASE 2] Searching for all %d nodes...\n", NUM_NODES * 3);
//     int found_count = 0;
//     int not_found = 0;
//     start = 0.0;
//     for (int i = 0; i < NUM_NODES; i++) {
//         // find data

//         Node *found_data = hash_search(hashmap, data_keys[i]);

//         // find array
//         Node *found_array = hash_search(hashmap, array_keys[i]);

//         //find promise
//         Node *found_promise = hash_search(hashmap, promise_keys[i]);

//         if (found_data) {
//             found_count++;
//             // Verify data integrity
//             if (found_data->value.data && 
//                 *ReadDataInt(found_data->value.data) != 1234) {
//                 printf("[X] Data mismatch at index %d!\n", i);
//             }
//         } else {
//             not_found++;
//             printf("[X] Could not find key: %s\n", data_keys[i]);
//         }
//         if (found_array) {
//             found_count++;
//             // Verify data integrity
//             if (!found_array->value.array) {
//                 printf("[X] Array mismatch at index %d!\n", i);
//             }
//         } else {
//             not_found++;
//             printf("[X] Could not find key: %s\n", data_keys[i]);
//         }
//         if (found_promise) {
//             found_count++;
//             // Verify data integrity
//             if (!found_promise->value.promise) {
//                 printf("[X] Promise mismatch at index %d!\n", i);
//             }
//         } else {
//             not_found++;
//             printf("[X] Could not find key: %s\n", data_keys[i]);
//         }
//     }
//     end = clock();
//     double search_time = ((double)(end - start)) / CLOCKS_PER_SEC;
//     printf("[✓] Found %d/%d nodes in %.3f seconds (%.0f searches/sec)\n", 
//            found_count, NUM_NODES * 3, search_time, NUM_NODES * 3 / search_time);
//     if (not_found > 0) {
//         printf("[X] WARNING: %d nodes not found!\n", not_found);
//     }
//     printf("[v] inserts performed old hashmap %ld\n", hashmap->inserts);
//     for (int x = 0; x < NUM_NODES; x++){
//         free(data_keys[x]);
//         free(array_keys[x]);
//         free(promise_keys[x]);
//     }
//     free(data_keys);
//     free(array_keys);
//     free(promise_keys);
//     start = 0.0;
//     Hashmap *new_hashmap = resize_hashmap(hashmap);
//     Hashmap *new_hashmap2 = resize_hashmap(new_hashmap);
//     Hashmap *new_hashmap3 = resize_hashmap(new_hashmap2);
//     Hashmap *new_hashmap4 = resize_hashmap(new_hashmap3);
//     end = clock();
//     if (!new_hashmap)
//         printf("[x] nooop\n");
//     double resize_time = ((double)(end - start)) / CLOCKS_PER_SEC;
//     printf("[✓] resize time %.3f\n", resize_time);
//     printf("[v]new size = %d\n", new_hashmap->size);
//     printf("[v] inserts performed new hashmap %ld\n", new_hashmap->inserts);
//     free_hashmap_and_data(hashmap);
//     free_hashmap_and_data(new_hashmap);
//     free_hashmap_and_data(new_hashmap2);
//     free_hashmap_and_data(new_hashmap3);
//     free_hashmap_and_data(new_hashmap4);
//    return -1;
    
// }
