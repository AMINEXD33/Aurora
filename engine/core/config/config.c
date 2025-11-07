#include "config.h"

// this file will contain getter functions , that just act
// as an easy way to get config values, feel free to add more
// if you added new configuration values 

// -10 is an exit code reserved for config value not found

cJSON *INIT_CORE_CONFIG(){
    File_object *fileobj =  open_file_read_mode(CORE_CONFIG_PATH);
    if (fileobj->is_error){
        printf("can't open %s config file : %s\n", CORE_CONFIG_PATH, fileobj->error);
        return NULL;
    }
    // alloc a buffer for the content
    char * buffer = read_file(fileobj);
    // parse it 
    cJSON *json = cJSON_Parse(buffer);
    // free buffer 
    free(buffer);
    free_close_fileobject(fileobj);
    return json;
}


int *GET_CORE_COUNT(cJSON *json){
    int *cores_count = get_nested_values(json, INT, 1, "cores");
    if (!cores_count){
        printf("[x] cores are not configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
}

int *GET_THREAD_COUNT(cJSON *json){
    int *thread_count = get_nested_values(json, INT, 1, "threads");
    if (!thread_count){
        printf("[x] threads are not configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
}

int *GET_SHARED_MEMORY_UNITES(cJSON *json){
    int *shared_memory_unites = get_nested_values(json, INT, 1, "shared_momory_unites");
    if (!shared_memory_unites){
        printf("[x] shared_memory_unites is not configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
}

int *GET_THRESHOLD(cJSON *json){
    int *threshold = get_nested_values(json, INT, 1, "threshold");
    if (!threshold){
        printf("[x] threshold are nto configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
}
int *GET_MIN_THRESHOLD(cJSON *json){
    int *min_threshold = get_nested_values(json, INT, 1, "min_threshold");
    if (!min_threshold){
        printf("[x] min_threshold are nto configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
}
int *GET_MAX_THRESHOLD(cJSON *json){
    int *max_threshold = get_nested_values(json, INT, 1, "max_threshold");
    if (!max_threshold){
        printf("[x] max_threshold are nto configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
}