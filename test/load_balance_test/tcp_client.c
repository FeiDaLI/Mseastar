//使用的不同的端口
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

int create_and_bind_socket() {
    int sockfd;
    struct sockaddr_in client_addr;
    // 创建 TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    // 设置客户端本地地址并绑定，让系统分配随机端口
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY; // 使用任意本地 IP
    client_addr.sin_port = 0; // 端口为0，表示由系统自动分配随机端口
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int main() {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    for (int i = 0; i < NUM_MESSAGES; i++) {
        int sockfd = create_and_bind_socket();

        printf("Client bound to a new random port for message %d\n", i);

        // 连接到服务器
        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connect failed");
            close(sockfd);
            continue;
        }

        snprintf(buffer, BUFFER_SIZE, "Message %d from client", i);
        printf("Sending: %s\n", buffer);

        // 发送消息
        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("Write failed");
        } else {
            // 接收回显
            int n = read(sockfd, buffer, BUFFER_SIZE);
            if (n <= 0) {
                perror("Read failed");
            } else {
                buffer[n] = '\0';
                printf("Received: %s\n", buffer);
            }
        }

        close(sockfd);
        usleep(100000); // 100ms 延迟以便观察
    }

    return 0;
}