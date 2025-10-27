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
    unsigned long int capacity;
    long int count;
    pthread_mutex_t lock;
    pthread_cond_t slot_available;
}PromiseStore;

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
File_object *init_fileobject();
void free_close_fileobject(File_object *fileobject);
char *getFileName(const char *path);
File_object * assign_error(File_object *fileobj);
File_object * open_file_read_mode(char *path);
char * read_file(File_object *fileobject);
Data *InitDataPoint();
int WriteData(Data *data, type type,void *value, bool owned);
char *ReadDataStr(Data *data);
int *ReadDataInt(Data *data);
bool *ReadDataBool(Data *data);
float *ReadDataFloat(Data *data);
double *ReadDataDouble(Data *data);
void FreeDataPoint(Data *data);









#endif