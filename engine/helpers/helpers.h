#ifndef HELPERS_HEADER
#define HELPERS_HEADER

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "../../global.h"
#include <cjson/cJSON.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <stdint.h>
#include "xxhash.h"
 

#define DATA_OWNED true 
#define DATA_NOT_OWNED false 

typedef enum {
    STRING = 11, 
    BOOLEAN = 12, 
    NONE = 13, 
    INT = 14, 
    FLOAT = 15, 
    DOUBLE = 16, 
    LONG = 17
} type;

typedef enum {
    DATA = 21, 
    ARRAY = 22, 
    NOTHING = 23, 
    PROMISE = 24, 
    NODE = 25
} complex_structures;


/*data representation*/

typedef struct {
    union {
        int int_val;
        long long_val;
        float float_val;
        double double_val;
        char* string_val;
        bool bool_val;
    } value;
    char *key; // not used normally, only when used as cache
    type type; 
    bool data_owned;
}Data;

/* Array */
typedef struct {
    Data **array;
    char *key; // not used normally, only when used as cache
    unsigned long int size;
    unsigned long int index;
}Array;


typedef enum {READY, COMPUTING, PENDING} status;

/*promis structure*/
typedef struct{
    char *key;
    status status;
    complex_structures type; 
    union{
        Data *data;
        Array *array;
    }datatype;
    pthread_mutex_t lock;
    pthread_cond_t ready;
    unsigned int waiting_threads;
    unsigned int working_threads;
    unsigned int access_count;
}Promise;


/** Binary tree node */
typedef struct Node{
    XXH64_hash_t hashed_key;
    complex_structures type; 
    union
    {
        Array *array;
        Data *data;
        Promise *promise;
    }value;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
    bool is_root;
} Node;

/** hashmap */
typedef struct
{
    Node **node;
    unsigned long int size;
    unsigned long int inserts;
}Hashmap;


/*promise store*/
typedef struct{
    Hashmap *hashmap;
    long int capacity;
    long int count;
    pthread_mutex_t lock;
    pthread_cond_t slot_available;
    // ema tracking
    double smoothing;
    double ema_occupancy;
    double prev_ema;

    // adaptive threshold
    double threshold;
    double min_threshold;
    double max_threshold;
}PromiseStore;

/** thread initials */
typedef struct{
    PromiseStore *store;
    int id;
    bool stop_flag;
}thread_info;

/*file structure*/
typedef struct{
    FILE *fptr;// file pointer
    char *filename;// filename
    char *filepath;// filepath
    bool is_error;// did an error accured
    char *error;// error message
}File_object;

/*json extracted arguments*/
typedef struct{
    unsigned int argnumber;
    void **args;
    char *types;
}Args;

typedef struct 
{
    _Atomic (Hashmap *) curr_table;
}hashmap_resize_info;

/*Json api*/
void *get_nested_values(cJSON *json,type type,  unsigned int argcount, ...);

/** Files API */
File_object *init_fileobject();
void free_close_fileobject(File_object *fileobject);
char *getFileName(const char *path);
File_object * assign_error(File_object *fileobj);
File_object * open_file_read_mode(char *path);
char * read_file(File_object *fileobject);

/** Datapoints API */
Data *InitDataPoint(char *key);
char *ReadDataStr(Data *data);
int *ReadDataInt(Data *data);
bool *ReadDataBool(Data *data);
float *ReadDataFloat(Data *data);
double *ReadDataDouble(Data *data);
void FreeDataPoint(Data *data);
int append_datapoint(Array *array, Data *appended_value);
void free_array(Array *array);
int resize(Array *array);
Array *InitDataPointArray(char *key);
void printArray(Array *arr);
void printDataPoint(Data *d, char *format);
int WriteDataString(Data *data,char *value);
int WriteDataInt(Data *data,int value);
int WriteDataFloat(Data *data,float value);
int WriteDataDouble(Data *data,double value);
int WriteDataBool(Data *data,bool value);
int WriteDataLong(Data *data,long value);

// promise
PromiseStore *InitPromiseStore(
    unsigned int size,
    double threshold,
    double min_threshold,
    double max_threshold
);

/** Promise API */
Promise *InitPromise(char *key);
void free_promise_store(PromiseStore *store);
Promise *get_create_promise(PromiseStore *store, char *key);
bool claim_work(Promise *promise);
void publishData(Promise *promise, Data *result);
void publishArray(Promise *promise, Array *result);
Promise *get_promise(PromiseStore *store, char *key);
Data *wait_for_result_data(Promise *promise);
Array *wait_for_result_array(Promise *promise);
void done_with_promise_data(Promise *promise);
double update_store_threshold(PromiseStore *store);
void free_promise(Promise *promise);

/** Serialization API */
size_t estimate_size_array_data(Array *arr);
void TagBuffer(uint8_t *buffer, size_t *offset);
size_t estimate_size_data(Data *d);
Array *deserialize_array_data(uint8_t *buffer, size_t *offset);
Data* deserialize_data(uint8_t *buffer, size_t *offset);
size_t serialize_array_of_data(Array *arr, uint8_t *buffer);
size_t serialize_data(Data *d, uint8_t *buffer);


/** Btree API */

Node *InitBtree();
void free_tree_bfs(Node *root);
Node *init_Array_node(Node *parent, XXH64_hash_t key_hash, Array *value);
Node *init_Promise_node(Node *parent, XXH64_hash_t key_hash, Promise *value);
Node *init_Data_node(Node *parent, XXH64_hash_t key_hash, Data *value);
int push_Promise_to_tree(Promise *data, Node *btree);
int push_Array_to_tree(Array *data, Node *btree);
int push_Data_to_tree(Data *data, Node *btree);
Node *get_Node_from_tree(char *key, Node *btree);
Promise *get_Promise_from_tree(char *key, Node *btree);
Array *get_Array_from_tree(char *key, Node *btree);
Data *get_Data_from_tree(char *key, Node *btree);
Node *free_node(Node*root,  Node *target_node);

/** helper for both Btree and Hashmap */
int explore_btree(
    Hashmap *new_hashmap, 
    Node *root, 
    int(do_something)(Hashmap* new_hashmap,  Node *current_node)
);

/** Hashmap API */
Hashmap* InitHashMap(unsigned long int initial_size);
Node *hash_search(Hashmap *hashmap, char* key);
void free_hashmap_and_data(Hashmap *hashmap);
int hash_push_Data(Hashmap *hashmap, Data *value);
int hash_push_Array(Hashmap *hashmap, Array *value);
int hash_push_Promise(Hashmap *hashmap, Promise *value);
Hashmap *resize_hashmap(Hashmap *old_hashmap);
void rand_str(char *dest, size_t length);
#endif