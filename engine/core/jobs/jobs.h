#ifndef JOBS_HEADERS
#define JOBS_HEADERS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
void *worker_thread(void *arg);
void *cleaner_thread(void *arg);
void *health_thread(void *arg);





#endif