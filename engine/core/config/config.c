#include "config.h"

// this file will contain getter functions , that just act
// as an easy way to get config values, feel free to add more
// if you added new configuration values 

// -10 is an exit code reserved for config value not found
// -11 is an exit code reserved for bad config
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


int GET_CORE_COUNT(cJSON *json){
    int *cores_count = get_nested_values(json, INT, 1, "cores");
    if (!cores_count){
        printf("[x] cores are not configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
    if (*cores_count < 1){
        printf("[x] core count must be >= 1\n");
        exit(-11);
    }
    int value = *cores_count;
    return value;
}

int GET_THREAD_COUNT(cJSON *json){
    int *thread_count = get_nested_values(json, INT, 1, "threads");
    if (!thread_count){
        printf("[x] threads are not configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
    if (*thread_count < 1){
        printf("[x] threads count must be >= 1\n");
        exit(-11);
    }
    int value = *thread_count;
    return value;
}

int GET_SHARED_MEMORY_UNITES(cJSON *json){
    int *shared_memory_units = get_nested_values(json, INT, 1, "shared_memory_units");
    if (!shared_memory_units){
        printf("[x] shared_memory_units is not configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
    if (*shared_memory_units < 1){
        printf("[x] shared memory unites must be >= 1\n");
        exit(-11);
    }
    int value = *shared_memory_units;
    return value;
}

double GET_THRESHOLD(cJSON *json){
    double *threshold = get_nested_values(json, DOUBLE, 1, "threshold");
    if (!threshold){
        printf("[x] threshold are nto configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
    if (*threshold < 1){
        printf("[x] shared memory unites must be >= 1\n");
        exit(-11);
    }
    double value = *threshold;
    return value;
}
double GET_MIN_THRESHOLD(cJSON *json){
    double *min_threshold = get_nested_values(json, DOUBLE, 1, "min_threshold");
    if (!min_threshold){
        printf("[x] min_threshold are nto configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
    if (*min_threshold < 1){
        printf("[x] min threshold  must be >= 1\n");
        exit(-11);
    }
    double value = *min_threshold;
    return value;
}
double GET_MAX_THRESHOLD(cJSON *json){
    double *max_threshold = get_nested_values(json, DOUBLE, 1, "max_threshold");
    if (!max_threshold){
        printf("[x] max_threshold are nto configured in the %s\n", CORE_CONFIG_PATH);
        exit(-10);
    }
    if (*max_threshold < 1){
        printf("[x] min threshold  must be >= 1\n");
        exit(-11);
    }
    double value = *max_threshold;
    return value;
}