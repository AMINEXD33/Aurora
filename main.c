#include "./engine/core/init/init.h"
#include "./engine/core/jobs/jobs.h"
#include "./engine/helpers/helpers.h"
#include "./engine/core/config/config.h"


int main (char **argc, int argv){
    cJSON *core_config =  INIT_CORE_CONFIG();
    int thread_count = GET_THREAD_COUNT(core_config);
    int core_count = GET_THREAD_COUNT(core_config);
    double threshold = GET_THRESHOLD(core_config);
    double min_threshold = GET_MIN_THRESHOLD(core_config);
    double max_threshold = GET_MAX_THRESHOLD(core_config);
    int shared_mem_unites = GET_SHARED_MEMORY_UNITES(core_config);
    
    printf("---------LOADED CONFIG-----------\n");
    printf("[@] thread count = %d\n", thread_count);
    printf("[@] core count = %d\n", core_count);
    printf("[@] threshold = %lf\n", threshold);
    printf("[@] min threshold = %lf\n", min_threshold);
    printf("[@] max threshold = %lf\n", max_threshold);
    printf("[@] shared memory unites  = %d\n", shared_mem_unites);
    printf("---------------------------------\n");

    INIT_jobs(
        thread_count,
        shared_mem_unites,
        threshold,
        min_threshold,
        max_threshold,
        worker_thread,
        cleaner_thread
    );

    cJSON_Delete(core_config);

    return 1;
}