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

#define DATA_OWNED true 
#define DATA_NOT_OWNED false 

typedef enum {STRING, BOOLEAN, NONE, INT, FLOAT, DOUBLE, LONG} type;

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

/*promise store*/
typedef struct{
    Promise **promises;
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

// functions

void *get_nested_values(cJSON *json,type type,  unsigned int argcount, ...);
// files
File_object *init_fileobject();
void free_close_fileobject(File_object *fileobject);
char *getFileName(const char *path);
File_object * assign_error(File_object *fileobj);
File_object * open_file_read_mode(char *path);
char * read_file(File_object *fileobject);

// data points


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
void free_promise_store(PromiseStore *store);
Promise *get_create_promise(PromiseStore *store, const char *key);
bool claim_work(Promise *promise);
void publishData(Promise *promise, Data *result);
void publishArray(Promise *promise, Array *result);
Promise *get_promise(PromiseStore *store, const char *key);
Data *wait_for_result_data(Promise *promise);
Array *wait_for_result_array(Promise *promise);

void done_with_promise_data(Promise *promise);
double update_store_threshold(PromiseStore *store);

size_t estimate_size_array_data(Array *arr);
void TagBuffer(uint8_t *buffer, size_t *offset);
size_t estimate_size_data(Data *d);
Array *deserialize_array_data(uint8_t *buffer, size_t *offset);
Data* deserialize_data(uint8_t *buffer, size_t *offset);
size_t serialize_array_of_data(Array *arr, uint8_t *buffer);
size_t serialize_data(Data *d, uint8_t *buffer);
#endif