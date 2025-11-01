#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

struct timeval *make_clock() {
    struct timeval *t = malloc(sizeof(struct timeval));
    return t;
}

double calc_delta(struct timeval *start,struct timeval *end){
    
    double delta = (end->tv_sec - start->tv_sec) + 
                   (end->tv_usec - start->tv_usec) * 1e-6;
    return delta;
}
int main() {
    struct timeval *start = make_clock();
    struct timeval *end = make_clock();

    gettimeofday(start, NULL);

    usleep(5000); // sleep 0.5 sec
    gettimeofday(end, NULL);

    double delta = calc_delta(start, end);
    printf("delta = %.6f seconds\n", delta);

    free(start);
    free(end);
    return 0;
}