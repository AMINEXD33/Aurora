#include "./jobs.h"
#include <time.h>
#include <unistd.h>
#include <sys/time.h>


void *health_thread(void *arg){

    printf("[+] health thread stuff ...\n");


    while (true){
        printf("<health check rootine>\n");
        sleep(10);
    }

    return NULL;
}