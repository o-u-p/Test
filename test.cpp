#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#define PORT_NUMBER 12345
#define MAX_BUFFER_SIZE 1024
int main() {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Bind the socket to a specific IP and port
    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections on any IP address
    servaddr.sin_port = htons(PORT_NUMBER); // Replace with your desired port number
    
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Server loop
    while (true) {
        char buffer[MAX_BUFFER_SIZE];
        struct sockaddr_in clientaddr{};
        socklen_t clientaddrlen = sizeof(clientaddr);
        
        // Receive data from the client
        ssize_t numBytes = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (numBytes < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }
        
        // Process the received data
        buffer[numBytes] = '\0'; // Add null terminator to make it a string
        std::cout << "Received data: " << buffer << std::endl;
        
        // Reply to the client
        const char *response = "Hello from server";
        
        // Connect the socket to the client address
        if (connect(sockfd, (const struct sockaddr *)&clientaddr, clientaddrlen) < 0) {
            perror("connect failed");
            exit(EXIT_FAILURE);
        }
        
        // Send response to the client using send instead of sendto
        ssize_t sentBytes = send(sockfd, response, strlen(response), 0);
        if (sentBytes < 0) {
            perror("send failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Close the socket
    close(sockfd);
    
    return 0;
}
