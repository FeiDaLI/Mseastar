//使用的同一个端口和ip
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_IP "127.0.0.1"
#define PORT 10000
#define BUFFER_SIZE 1024
#define NUM_MESSAGES 20

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 创建 TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 发送多条消息以测试负载均衡
    for (int i = 0; i < NUM_MESSAGES; i++) {
        snprintf(buffer, BUFFER_SIZE, "Message %d from client", i);
        printf("Sending: %s\n", buffer);

        // 发送消息
        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("Write failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // 接收回显
        int n = read(sockfd, buffer, BUFFER_SIZE);
        if (n <= 0) {
            perror("Read failed");
            break;
        }

        buffer[n] = '\0';
        printf("Received: %s\n", buffer);

        // 稍微延迟以便观察
        usleep(100000); // 100ms
    }

    close(sockfd);
    return 0;
}