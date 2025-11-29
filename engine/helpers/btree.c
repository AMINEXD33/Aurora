#include "./helpers.h"
#include "stdlib.h"   /* abort() */
#include "xxhash.h"

Node *InitBtree(){
    Node *root = calloc(1, sizeof(Node));
    if (!root)
        return NULL;
    root->left = NULL;
    root->right = NULL;
    root->parent = NULL;
    root->value.array = NULL;
    root->value.data = NULL;
    root->type = NOTHING;
    root->is_root = true;
    return root;
}
Node *init_node(Node *parent, XXH64_hash_t key_hash,  complex_structures type, void *value){
    Node *node = calloc(1, sizeof(Node));
    if (!node)
        return NULL;
    node->hashed_key = key_hash;
    node->type = type;
    if (type == DATA)
        node->value.data = (Data *)value;
    else if(type == ARRAY)
        node->value.array = (Array *)value;
    else if(type == PROMISE)
        node->value.promise = (Promise *)value;
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;
    node->is_root = false;
    return node;
}

void write_to_node(Node *node, XXH64_hash_t key_hash, 
    complex_structures type, void *value){
    node->type = type;
    node->hashed_key = key_hash;
    if (node->type == DATA)
        node->value.data = (Data *)value;
    else if (node->type == ARRAY)
        node->value.array = (Array *)value;
}
void free_node_resources(Node *target_node){

    // Free the data
    if (target_node->type == DATA && target_node->value.data) {
        FreeDataPoint(target_node->value.data);
    } else if (target_node->type == ARRAY && target_node->value.array) {
        free_array(target_node->value.array);
    } else if (target_node->type == PROMISE && target_node->value.promise) {
        if (target_node->value.promise->datatype.array)
            free_array(target_node->value.promise->datatype.array);
        if (target_node->value.promise->datatype.data)
            FreeDataPoint(target_node->value.promise->datatype.data);
        free(target_node->value.promise);
    }
    free(target_node);
    return;
}
void cpy_node_content(Node *src, Node *dst){
    if (!src || !dst){
        printf("[ERROR] to copy a node you need src and dst\n");
        return;
    }
    dst->hashed_key = src->hashed_key;
    dst->type = src->type;
    dst->value = src->value;
    return;
}
void swap_two_nodes_content(Node *nodeA, Node *nodeB){
    Node tmp_node;
    cpy_node_content(nodeA, &tmp_node);
    cpy_node_content(nodeB, nodeA);
    cpy_node_content(&tmp_node, nodeB);    
}

/**
 * check if a node is a left or right decendent of
 * a root node(root node can be any node with children)
 * ### return:
 *  `1`: for left
 *  `0`: for right
 *  `-1`: not connected
 */
int am_i_left_or_right(Node *parent, Node *child){
    if (!parent || !child) {
        printf("[ERROR] NULL parent or child\n");
        return -1;
    }
    
    if (parent->left == child) {
        return 1;  // left
    } else if (parent->right == child) {
        return 0;  // right
    } else {
        printf("[ERROR] Child %p is not connected to parent %p\n", child, parent);
        return -1;
    }
}

Node *free_node(Node*root,  Node *target_node){
    if (!target_node){
        printf("[ERROR] no target node\n");
        return root;    
    }
    bool has_left = target_node->left != NULL;
    bool has_right = target_node->right != NULL;

    // node has no left and no right
    if (!has_left && !has_right){
        //printf("case I\n");
        if (target_node->parent){
            Node *new_root = target_node->parent;
            // just swap parent to point to the left child
            int l_or_r_of_parent = am_i_left_or_right(target_node->parent, target_node);
            // right
            if (l_or_r_of_parent == 0){
                target_node->parent->right = NULL;
            }
            // left
            else{
                target_node->parent->left = NULL;
            }
            target_node->parent = NULL;
            //printf("deleting the leaf with a parent key %llu\n", target_node->hashed_key);
            free_node_resources(target_node);
            return root;
        }
        target_node->parent = NULL;
        //printf("deleting the only node with key %llu\n", target_node->hashed_key);
        free_node_resources(target_node);
        return NULL;// this is the only node
    }
    // node has no right side and it's not root
    if (has_left && !has_right && !target_node->is_root){
        //printf("case II\n");
        // just swap parent to point to the left child
        int l_or_r_of_parent = am_i_left_or_right(target_node->parent, target_node);
        // right
        if (l_or_r_of_parent == 0){
            target_node->parent->right = target_node->left;
            target_node->left->parent = target_node->parent;
        }
        // left
        else{
            target_node->parent->left = target_node->left;
            target_node->left->parent = target_node->parent;
        }
        // isolate the target
        target_node->parent = NULL;
        target_node->left = NULL;
        //printf("deleting node with key %llu\n", target_node->hashed_key);
        free_node_resources(target_node);
        return root;
    }else if(has_left && !has_right && target_node->is_root){
        //printf("case III\n");
        Node *new_root = target_node->left;
        new_root->parent = NULL;
        new_root->is_root = true;
        
        // isolate old root
        target_node->left = NULL;
        target_node->right = NULL;
        
        //printf("deleting node with key %llu\n", target_node->hashed_key);
        free_node_resources(target_node);
        return new_root;
    }
    // node has right side
    // not that here we don't need to update any pointers othern than the leaf
    if (has_right){
            //printf("case IV\n");
            // Find in-order successor (leftmost in right subtree)
            Node *curr = target_node->right;
            
            // Go as far left as possible
            while (curr->left != NULL) {
                curr = curr->left;
            }
            
            // Now curr is the successor
            swap_two_nodes_content(curr, target_node);

            // Now delete curr (which has at most a right child)
            // If curr has a right child, promote it
            if (curr->right) {
                int l_or_r = am_i_left_or_right(curr->parent, curr);
                if (l_or_r == 0) {
                    curr->parent->right = curr->right;
                } else {
                    curr->parent->left = curr->right;
                }
                curr->right->parent = curr->parent;
            } else {
                // curr is a leaf
                int l_or_r = am_i_left_or_right(curr->parent, curr);
                if (l_or_r == 0) {
                    curr->parent->right = NULL;
                } else {
                    curr->parent->left = NULL;
                }
            }
            
            curr->parent = NULL;
            curr->right = NULL;
            free_node_resources(curr);
            
            return root;
    }
}
void *get_value_from_tree(char *key, Node *btree, complex_structures type){
    Node *curr = btree;
    int how_many_recurse = 0;
    XXH64_hash_t key_hash = XXH64(key, strlen(key), 0);
    while (curr != NULL){
        // root node is empty insert here
        //printf("[NN] recurse %d\n", how_many_recurse);
        if (curr->is_root && curr->type == NOTHING){
            return NULL;
        }

        if (curr->hashed_key == key_hash){
            if (type == DATA)
                return curr->value.data;
            else if(type == ARRAY)
                return curr->value.array;
            else if(type == PROMISE)
                return curr->value.promise;
            else if(type == NODE)
                return curr;
            else
                return NULL;
        }
        bool go_left = key_hash <= curr->hashed_key;
        bool go_right = key_hash > curr->hashed_key;
        // left or right are not empty recurse and go to the next node
        if (go_right && curr->right){
            //printf("[v] recursing right\n");
            how_many_recurse++;
            curr = curr->right;
        }
        else if (go_left && curr->left){
            //printf("[v] recursing left\n");
            how_many_recurse++;
            curr = curr->left;
        }
        else{
            return NULL;
        }
    }
    //printf("[V] done!\n");
    return NULL;
}
Data *get_Data_from_tree(char *key, Node *btree){
    Data * data = (Data *)get_value_from_tree(key, btree, DATA);
    return data;
}
Array *get_Array_from_tree(char *key, Node *btree){
    Array *array = (Array *)get_value_from_tree(key, btree, ARRAY);
    return array;
}
Promise *get_Promise_from_tree(char *key, Node *btree){
    Promise *promise = (Promise *)get_value_from_tree(key, btree, PROMISE);
    return promise;
}
void push_Data_to_tree(Data *data, Node *btree){
    if (!btree || !data || !btree->is_root){
        return;
    }
    Node *curr = btree;
    XXH64_hash_t key_hash = XXH64(data->key, strlen(data->key), 0);
    while (curr != NULL){
        // root node is empty insert here
        if (curr->is_root && curr->type == NOTHING){
            write_to_node(curr, key_hash, DATA, data);
            //printf("[v] inserting to the root\n");
            //printf("value = %llu  \n", curr->hashed_key);
            return;
        }
        bool go_left = key_hash <= curr->hashed_key;
        bool go_right = key_hash > curr->hashed_key;
        // is value to the left
        if (go_left && curr->left == NULL){
            // create node
            Node *node = init_node(curr, key_hash, DATA, data);
            if (!node){
                //printf("[x] can't create node\n");
                return;
            }
            //printf("[v] inserting to the left\n");
            //printf("value = %llu   with parent %llu\n", node->hashed_key, curr->hashed_key);
            curr->left = node;
            return;
        }
        // is to right
        else if (go_right  && curr->right == NULL){
            // create node
            Node *node = init_node(curr, key_hash, DATA, data);
            if (!node){
                //printf("[x] can't create node\n");
                return;
            }
            //printf("[v] inserting to the right\n");
            //printf("value = %llu   with parent %llu\n", node->hashed_key, curr->hashed_key);
            curr->right = node;
            return;
        }
        // left or right are not empty recurse and go to the next node
        if (go_right && curr->right){
            //printf("[v] recursing right\n");
            curr = curr->right;
        }
        else if (go_left && curr->left){
            //printf("[v] recursing left\n");
            curr = curr->left;
        }
        else{
            return;
        }
    }
    //printf("[V] done!\n");
    return;
}
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>



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

// int main(){
//     Node *btree = InitBtree();
//     Data *dt = InitDataPoint("theada213");
//     WriteDataFloat(dt, 112414.141414);
//     Data *dt2 = InitDataPoint("sdgdfgdfg");
//     WriteDataFloat(dt2, 112414.141414);
//     Data *dt3 = InitDataPoint("gxcvxcv");
//     WriteDataFloat(dt3, 112414.141414);
//     Data *dt4 = InitDataPoint("hghrty");
//     WriteDataFloat(dt4, 112414.141414);
//     Data *dt5 = InitDataPoint("qweqdsfsd");
//     WriteDataFloat(dt5, 112414.141414);
//     Data *dt6 = InitDataPoint("aminemeftah2214");
//     WriteDataFloat(dt6, 112414.141414);
    
//     push_Data_to_tree(dt, btree);
//     push_Data_to_tree(dt2, btree);
//     push_Data_to_tree(dt3, btree);
//     push_Data_to_tree(dt4, btree);
//     push_Data_to_tree(dt5, btree);
//     push_Data_to_tree(dt6, btree);

//     Node *wanted = get_value_from_tree("theada213", btree, NODE);
//     Node *wanted2 = get_value_from_tree("sdgdfgdfg", btree, NODE);
//     Node *wanted3 = get_value_from_tree("gxcvxcv", btree, NODE);
//     Node *wanted4 = get_value_from_tree("hghrty", btree, NODE);
//     Node *wanted5 = get_value_from_tree("qweqdsfsd", btree, NODE);
//     Node *wanted6 = get_value_from_tree("aminemeftah2214", btree, NODE);
    
//     if (wanted && wanted2 && wanted3 && wanted4 && wanted5 && wanted6){
//         printf("[+] all data was found\n");
//     }
//     Node *root = free_node(btree,wanted);

//     wanted2 = get_value_from_tree("sdgdfgdfg", root, NODE);
//     root = free_node(root,wanted2);

//     wanted3 = get_value_from_tree("gxcvxcv", root, NODE);
//     root = free_node(root,wanted3);

//     wanted4 = get_value_from_tree("hghrty", root, NODE);
//     root = free_node(root,wanted4);

//     wanted5 = get_value_from_tree("qweqdsfsd", root, NODE);
//     root =free_node(root,wanted5);

//     wanted6 = get_value_from_tree("aminemeftah2214", root, NODE);
//     root = free_node(root,wanted6);

//     free(dt);
//     free(dt2);
//     free(dt3);
//     free(dt4);
//     free(dt5);
//     free(dt6);
//     return 0;
// } 

void free_tree_bfs(Node *root) {
    if (!root) return;
    
    // Use a simple array as queue
    size_t capacity = 256;
    size_t size = 0;
    size_t front = 0;
    size_t totalfreed =0;
    Node **queue = malloc(capacity * sizeof(Node*));
    
    if (!queue) {
        printf("[ERROR] Can't allocate queue\n");
        return;
    }
    
    queue[size++] = root;
    
    while (front < size) {
        Node *current = queue[front++];
        
        // Add children to queue before freeing
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
        
        // Free the data
        if (current->type == DATA && current->value.data) {
            FreeDataPoint(current->value.data);
        } else if (current->type == ARRAY && current->value.array) {
            free_array(current->value.array);
        } else if (current->type == PROMISE && current->value.promise) {
            if (current->value.promise->datatype.array)
                free_array(current->value.promise->datatype.array);
            if (current->value.promise->datatype.data)
                FreeDataPoint(current->value.promise->datatype.data);
            free(current->value.promise);
        }
        
        //printf("Freed node with key %llu\n", current->hashed_key);
        free(current);
        totalfreed++;
    }
    printf("[+]Total free calls are %ld\n", totalfreed);
    free(queue);
}


#include <time.h>



int main() {
    srand(time(NULL));
    
    const int NUM_NODES = 100000;
    printf("[*] Starting stress test with %d nodes...\n", NUM_NODES);
    
    Node *btree = InitBtree();
    if (!btree) {
        printf("[X] Failed to init tree\n");
        return -1;
    }
    
    // Allocate arrays to store keys and data pointers
    char **keys = malloc(NUM_NODES * sizeof(char*));
    Data **data_array = malloc(NUM_NODES * sizeof(Data*));
    
    if (!keys || !data_array) {
        printf("[X] Failed to allocate test arrays\n");
        return -1;
    }
    
    // ========== PHASE 1: INSERT 100K NODES ==========
    printf("\n[PHASE 1] Inserting %d nodes...\n", NUM_NODES);
    clock_t start = clock();
    
    for (int i = 0; i < NUM_NODES; i++) {
        // Generate unique key
        keys[i] = malloc(32);
        snprintf(keys[i], 32, "key_%d_%d", i, rand());
        
        // Create data
        data_array[i] = InitDataPoint(keys[i]);
        WriteDataInt(data_array[i], i);
        
        // Insert into tree
        push_Data_to_tree(data_array[i], btree);
        
        if (i % 10000 == 0) {
            printf("  Inserted %d nodes...\n", i);
        }
    }
    
    clock_t end = clock();
    double insert_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("[✓] Inserted %d nodes in %.3f seconds (%.0f nodes/sec)\n", 
           NUM_NODES, insert_time, NUM_NODES / insert_time);
    
    // ========== PHASE 2: SEARCH ALL NODES ==========
    printf("\n[PHASE 2] Searching for all %d nodes...\n", NUM_NODES);
    start = clock();
    
    int found_count = 0;
    int not_found = 0;
    
    for (int i = 0; i < NUM_NODES; i++) {
        Node *found = get_value_from_tree(keys[i], btree, NODE);
        if (found) {
            found_count++;
            // Verify data integrity
            if (found->value.data && 
                *ReadDataInt(found->value.data) != i) {
                printf("[X] Data mismatch at index %d!\n", i);
            }
        } else {
            not_found++;
            printf("[X] Could not find key: %s\n", keys[i]);
        }
        
        if (i % 10000 == 0 && i > 0) {
            printf("  Searched %d nodes...\n", i);
        }
    }
    
    end = clock();
    double search_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("[✓] Found %d/%d nodes in %.3f seconds (%.0f searches/sec)\n", 
           found_count, NUM_NODES, search_time, NUM_NODES / search_time);
    
    if (not_found > 0) {
        printf("[X] WARNING: %d nodes not found!\n", not_found);
    }

    // ========== PHASE 3: DELETE RANDOM NODES ==========
    printf("\n[PHASE 3] Deleting random 50%% of nodes...\n");
    start = clock();
    
    int delete_count = NUM_NODES / 2;
    int deleted = 0;
    Node *root = btree;
    
    // Track which indices were deleted
    bool *was_deleted = calloc(NUM_NODES, sizeof(bool));  // ✅ Use a boolean array
    if (!was_deleted) {
        printf("[X] Failed to allocate deletion tracker\n");
        return -1;
    }
    
    // Shuffle indices for random deletion
    int *indices = malloc(NUM_NODES * sizeof(int));
    for (int i = 0; i < NUM_NODES; i++) {
        indices[i] = i;
    }
    
    //Fisher-Yates shuffle
    for (int i = NUM_NODES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
    
    // Delete first half of shuffled indices
    for (int i = 0; i < delete_count; i++) {
        int idx = indices[i];
        Node *to_delete = get_value_from_tree(keys[idx], root, NODE);
        
        if (to_delete) {
            root = free_node(root, to_delete);
            was_deleted[idx] = true;  // ✅ Mark as deleted
            deleted++;
            
            // if (deleted % 5000 == 0) {
            //     printf("  Deleted %d nodes...\n", deleted);
            // }
        }
    }
    
    end = clock();
    double delete_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("[✓] Deleted %d nodes in %.3f seconds (%.0f deletions/sec)\n", 
           deleted, delete_time, deleted / delete_time);
    
    
    // ========== PHASE 4: VERIFY REMAINING NODES ==========
    printf("\n[PHASE 4] Verifying remaining nodes...\n");
    start = clock();
    
    int should_exist = 0;
    int should_not_exist = 0;
    int correct = 0;
    int incorrect = 0;
    
    for (int i = 0; i < NUM_NODES; i++) {
        Node *found = get_value_from_tree(keys[i], root, NODE);
        
        if (was_deleted[i]) {  // ✅ O(1) lookup instead of O(n)
            should_not_exist++;
            if (!found) {
                correct++;
            } else {
                incorrect++;
            }
        } else {
            should_exist++;
            if (found) {
                correct++;
            } else {
                incorrect++;
            }
        }
        
        if (i % 10000 == 0 && i > 0) {
            printf("  Verified %d nodes...\n", i);
        }
    }
    
    end = clock();
    double verify_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("[✓] Verification: %d correct, %d incorrect (%.3f sec)\n", 
           correct, incorrect, verify_time);
    printf("    Should exist: %d | Should not exist: %d\n", 
           should_exist, should_not_exist);
    
    // ========== PHASE 5: CLEANUP ==========
    printf("\n[PHASE 5] Cleaning up remaining tree...\n");
    start = clock();
    
    free_tree_bfs(root);
    
    end = clock();
    double cleanup_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("[✓] Tree cleanup completed in %.3f seconds\n", cleanup_time);
    
    // Free test arrays
    for (int i = 0; i < NUM_NODES; i++) {
        free(keys[i]);
    }
    free(keys);
    free(data_array);
    free(indices);
    free(was_deleted);  // ✅ Don't forget this!
    
    // ========== SUMMARY ==========
    printf("\n========== STRESS TEST SUMMARY ==========\n");
    printf("Total nodes tested: %d\n", NUM_NODES);
    printf("Insert time: %.3f sec (%.0f nodes/sec)\n", insert_time, NUM_NODES/insert_time);
    printf("Search time: %.3f sec (%.0f searches/sec)\n", search_time, NUM_NODES/search_time);
    printf("Delete time: %.3f sec (%.0f deletions/sec)\n", delete_time, deleted/delete_time);
    printf("Verify time: %.3f sec\n", verify_time);
    printf("Cleanup time: %.3f sec\n", cleanup_time);
    printf("Total time: %.3f sec\n", 
           insert_time + search_time + delete_time + verify_time + cleanup_time);
    
    if (incorrect == 0 && should_not_exist == NUM_NODES/2 && should_exist == NUM_NODES/2) {
        printf("\n[✓✓✓] ALL TESTS PASSED! Tree is solid! [✓✓✓]\n");
    } else {
        printf("\n[XXX] TESTS FAILED: %d errors detected [XXX]\n", incorrect);
    }
    
    return 0;
}