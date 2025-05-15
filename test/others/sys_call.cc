#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define SYS_getpid 39

// 使用 clock_gettime 测量时间
double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + 
           (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    struct timespec start, end;
    int i;

    // 预热（可选）
    for(i = 0; i < 1000; i++) {
        syscall(SYS_getpid); // 示例系统调用
    }

    // 测量多次系统调用的总时间
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(i = 0; i < 10000000; i++) {
        syscall(SYS_getpid); // 示例系统调用
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double total_time = get_time_diff(start, end);
    double avg_time = total_time / 10000000;

    printf("总时间: %.6f 秒\n", total_time);
    printf("平均每次系统调用时间: %.6f 秒\n", avg_time);

    return 0;
}