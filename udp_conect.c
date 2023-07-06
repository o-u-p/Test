#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "172.22.33.209"  // Replace with the server IP address
#define SERVER_PORT 12345      // Replace with the server port number

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("inet_pton error");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect error");
        exit(EXIT_FAILURE);
    }

    // Send data to the server
    const char* message = "Hello, server!";
    for (size_t i = 0; i < 10000; i++)
    {
    if (send(sockfd, message, strlen(message), 0) < 0) {
        perror("send error");
        exit(EXIT_FAILURE);
    }
    }

    printf("Message sent to the server.\n");

    // Close the socket
    close(sockfd);

    return 0;
}
