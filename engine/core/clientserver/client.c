#include "./clientserver.h"

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

/**
 * just a test and show of concept function of how a work flow example
 */
int test_case(redisContext *c) {
    // claim work
    char chars0[3];
    rand_str(chars0, 2);
    Data *data = InitDataPoint(chars0);
    WriteDataFloat(data, 12141.151);
    int found = 0;
    Data *data_retrieved = get_Data_from_cache(c, chars0);
    //printf("[V] data was sent\n");
    if (data_retrieved){
        //printf("PRINTING DATA RECIEVED\n");
        //printDataPoint(data_retrieved, "\n");
        //printf("data key : %s\n", data_retrieved->key);
        FreeDataPoint(data_retrieved);
        found++;
    }else{
        // cache miss
        cache_Data(c, data, chars0);
    }


    char chars[10];
    rand_str(chars, 9);
    Data *data1 = InitDataPoint("data1");
    WriteDataFloat(data1, 121441.151);
    Data *data2 = InitDataPoint("data2");
    WriteDataFloat(data2, 6565.151);
    Data *data3 = InitDataPoint("data3");
    WriteDataFloat(data3, 68678.151);
    Data *data4 = InitDataPoint("data4");
    WriteDataFloat(data4, 234645.151);
    Array *arr = InitDataPointArray(chars);
    append_datapoint(arr, data1);
    append_datapoint(arr, data2);
    append_datapoint(arr, data3);
    append_datapoint(arr, data4);
    /** WORK FLOW TO CACHE A COMPUTE VALUE */
    // claim the work so no other process recomputes it


    // get the cached value
    Array *arr2 =  get_Array_from_cache(c, chars);
    if (arr2){
        //printf("PRINTING ARRAY RECIEVED\n");
        //printArray(arr2);
        free_array(arr2);
        found++;
    }
    else{
        // cache miss 
        cache_Array(c, arr, chars);
        //printf("[XXXXX] READY but nothing is returning(ARRAY)\n");
    }
    
    FreeDataPoint(data);
    free_array(arr);
    return found;
}

int sendstuff() {
    unsigned int data_misses = 0;
    unsigned int test_size = 12;
    unsigned int counter = test_size;
    redisContext *c = create_redis_conn();
    if (!c){
        printf("can't connect to server\n");
        return -1;
    }
    clock_t start = clock();
    while (counter > 1){
        data_misses +=  abs(test_case(c) - 2);
        counter--;
    }
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    redisFree(c);
    printf("[REPPORT]\n");
    printf("total cache misses = %d\n", data_misses);
    printf("percent of data misses = %lf%\n", (((double)data_misses/2) /((double)test_size-1)) * (double)100);
    printf("Execution time: %f seconds\n", elapsed);
}