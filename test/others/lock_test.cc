#include <pthread.h>
#include <time.h>
#include <stdio.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void measure_lock_time() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_mutex_lock(&mutex);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Lock time: %.9f seconds\n", elapsed_time);

    pthread_mutex_unlock(&mutex);
}

int main(){
    measure_lock_time();
}