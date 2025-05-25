#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 10000
#define NUM_THREADS 8
#define BUFFER_SIZE 1024

void *handle_client(void *arg) {
    int thread_id = *(int *)arg;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    // 设置 CPU 亲和性
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id % 8, &cpuset); // 绑定到 CPU 核 0 到 3
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_setaffinity_np failed");
        return NULL;
    }

    // 创建 TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    // 设置 SO_REUSEPORT 选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt SO_REUSEPORT failed");
        close(server_fd);
        return NULL;
    }

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定 socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return NULL;
    }

    // 监听连接
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        return NULL;
    }

    printf("Thread %d bound to CPU %d, listening on port %d\n", thread_id, thread_id % 4, PORT);

    while (1) {
        // 接受客户端连接
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        // 处理客户端数据
        while (1) {
            int n = read(client_fd, buffer, BUFFER_SIZE);
            if (n <= 0) {
                if (n == 0) {
                    printf("Thread %d (CPU %d): Client disconnected\n", thread_id, thread_id % 4);
                } else {
                    perror("Read failed");
                }
                break;
            }

            buffer[n] = '\0';
            printf("Thread %d (CPU %d) received: %s\n", thread_id, thread_id % 4, buffer);

            // 回显数据
            if (write(client_fd, buffer, n) < 0) {
                perror("Write failed");
                break;
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // 创建 4 个线程
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, handle_client, &thread_ids[i]) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    // 等待线程结束
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}