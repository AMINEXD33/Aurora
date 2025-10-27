#ifndef INIT_HEADERS
#define INIT_HEADERS 

#include "../../helpers/helpers.h"
#include <math.h>
// shared memory structure
typedef struct
{
    char **keys;
    Data **data;
    unsigned long int size;
    unsigned long int position;
}Shared;
// model structure
typedef struct{
    char *model_name;
    char *shared_object_name;
    char *version;
    char *author;
    char *definition;
    char *contact_info;
    unsigned int in_use_count;// this model is used how many time ?
}Model;
// job (thread) structure
typedef struct{
    unsigned int id;
    Model **models;

}Job;
// jobs (threads)
typedef struct 
{
    unsigned int thread_count;
    Job **jobs;
}Jobs;
Shared *InitSharedMemory();
void FreeSharedMemory(Shared *shared);
int append_to_share(Shared *share, char* key, Data *datapoint);






#endif