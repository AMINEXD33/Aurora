#include "../../helpers/helpers.h"
#include "./init.h"
#include <math.h>

/**
 * Initialize a shared memory structure , it will store and track 
 * objects of type Data dynamically
 * ### return: 
 *  `Shared *`: if successful
 *  `NULL`: on error
 */
Shared *InitSharedMemory(){
    unsigned long int size = 1;
    Shared *shared = malloc(sizeof(Shared));
    if (!shared){
        printf("can't allocate shared memory\n");
        return NULL;
    }
    shared->data = calloc(size, sizeof(Data *));
    if (!shared->data){
        printf("can' allocate memory for the data\n");
        free(shared);
        return NULL;
    }
    shared->keys = calloc(size, sizeof(char *));
    if (!shared->keys){
        printf("can't allocate memory for the keys\n");
        free(shared->data);
        free(shared);
        return NULL;
    }
    shared->position = 0;
    shared->size = size;
    return shared;
}

/**
 * free the shared memory structure
 */
void FreeSharedMemory(Shared *shared){
    if (!shared){
        return;
    }
    // free the slots
    for (unsigned long int index = 0; index < shared->position; index++){
        if (shared->data[index]){
            // data has it's own free logic
            FreeDataPoint(shared->data[index]);
        }
        free(shared->keys[index]);
    }
    // free pointers
    free(shared->data);
    free(shared->keys);
    // free structure
    free(shared);
}

/**
 * append a <key>:<datapoint> to the shared memory,
 * both the key and datapoint must be valid
 * the key it self will get copied, so you can pass a stack allocated
 * key or heap allocated key(you're responsible for this one)
 * ### return:
 *  `1`: if successful
 *  `-1`: on error
 */
int append_to_share(Shared *share, char* key, Data *datapoint){
    if (!share){
        printf("no shared memory pointer was passed!\n");
        return -1;
    }
    if (!key){
        printf("no key was passed!\n");
        return -1;
    }
    if (!datapoint){
        printf("no datapoint was passed!\n");
        return -1;
    }
    // new size
    unsigned long int new_size = ceil((share->size + 1) * 1.4);
    // if size is insufficient
    if ((share->position+1) >= share->size){
        // reallocate for both
        Data **data = realloc(share->data, new_size * sizeof(Data *));
        char **keys = realloc(share->keys, new_size * sizeof(char *));
        if (!share->data){
            printf("can't realloc the new data memory, old is kept\n");
            return -1;
        }
        if (!share->keys){
            printf("can't realloc the new keys memory ,old is kept\n");
            return -1;
        }
        // update the pointers
        share->data = data;
        share->keys = keys;
        // update size
        share->size = new_size;
        // init the new slots
        for (unsigned long int index = share->position; index < share->size; ++index){
            share->data[index] = NULL;
            share->keys[index] = NULL;
        }
    }
    // append data
    share->data[share->position] = datapoint;
    // copy the key
    char *key_cpy = calloc(strlen(key)+1, sizeof(char));
    // append key
    share->keys[share->position] = key_cpy;
    // update position
    share->position = share->position + 1;
    
    return 1;
}




