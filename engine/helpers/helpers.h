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
#define DATA_OWNED true 
#define DATA_NOT_OWNED false 

typedef enum {STRING, BOOLEAN, NONE, INT, FLOAT, DOUBLE} type;
/*data representation*/
typedef struct {
    void *value;
    type type; 
    bool data_owned;
}Data;

typedef enum {READY, COMPUTING, PENDING} status;
/*promis structure*/
typedef struct{
    char *key;
    status status;
    Data *data;
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

// files
File_object *init_fileobject();
void free_close_fileobject(File_object *fileobject);
char *getFileName(const char *path);
File_object * assign_error(File_object *fileobj);
File_object * open_file_read_mode(char *path);
char * read_file(File_object *fileobject);

// data points
Data *InitDataPoint();
int WriteData(Data *data, type type,void *value, bool owned);
char *ReadDataStr(Data *data);
int *ReadDataInt(Data *data);
bool *ReadDataBool(Data *data);
float *ReadDataFloat(Data *data);
double *ReadDataDouble(Data *data);
void FreeDataPoint(Data *data);

// promise
PromiseStore *InitPromiseStore();
void free_promise_store(PromiseStore *store);
Promise *get_create_promise(PromiseStore *store, const char *key);
bool claim_work(Promise *promise);
void publish(Promise *promise, Data *result);
Data *wait_for_result(Promise *promise);
void done_with_promise_data(Promise *promise);
void *cleaner_thread(void *arg);


#endif