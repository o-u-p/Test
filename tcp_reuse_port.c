#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define MAX_THREADS 2
#define BUFFER_SIZE 1024

int globalFd = -1;
bool startConnect = false;

void* threadFunc(void* arg) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        pthread_exit(NULL);
    }

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("Error setting SO_REUSEPORT");
        close(sockfd);
        pthread_exit(NULL);
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    if (bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    if (listen(sockfd, 10) < 0) {
        perror("Error listening on socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("Thread %ld: Waiting for connections...\n", pthread_self());

    while (1) {
		struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSockfd < 0) {
            perror("Error accepting connection");
            continue;
        }
		memcpy(arg, &clientAddress, clientAddressLength);
        // Get client IP address and port
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddress.sin_port);

        printf("Thread %ld: New connection accepted from %s:%d\n", pthread_self(), clientIP, clientPort);

        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;
        while ((bytesRead = read(clientSockfd, buffer, BUFFER_SIZE)) > 0) {
            printf("Thread %ld: Received data from %s:%d - %.*s\n", pthread_self(), clientIP, clientPort, (int)bytesRead, buffer);
			globalFd = clientSockfd;
			startConnect = true;
            // Add your custom logic to process the received data here
        }

        if (bytesRead == 0) {
            printf("Thread %ld: Connection closed by client %s:%d\n", pthread_self(), clientIP, clientPort);
        } else {
            perror("Error reading from socket");
        }

        //close(clientSockfd);
    }

    close(sockfd);
    pthread_exit(NULL);
}

void* threadFunc2(void* arg) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        pthread_exit(NULL);
    }

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("Error setting SO_REUSEPORT");
        close(sockfd);
        pthread_exit(NULL);
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    if (bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    if (listen(sockfd, 10) < 0) {
        perror("Error listening on socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("Thread %ld: Waiting for connections...\n", pthread_self());

    {
		while (!startConnect)
		{
			if (startConnect)
			{
				break;
			}
		}

		struct sockaddr_in clientAddress = *(struct sockaddr_in*)arg;

        // Get client IP address and port
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddress.sin_port);

        printf("Thread %ld: New connection accepted from %s:%d\n", pthread_self(), clientIP, clientPort);

		socklen_t clientAddressLength = sizeof(clientAddress);
        int ret = connect(globalFd, (struct sockaddr*)&clientAddress, clientAddressLength);
        if (ret < 0) {
            perror("Error connection");
            // return NULL;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;
        while ((bytesRead = read(globalFd, buffer, BUFFER_SIZE)) > 0) {
            printf("Thread %ld: Received data from %s:%d - %.*s\n", pthread_self(), clientIP, clientPort, (int)bytesRead, buffer);

            // Add your custom logic to process the received data here
        }

        if (bytesRead == 0) {
            printf("Thread %ld: Connection closed by client %s:%d\n", pthread_self(), clientIP, clientPort);
        } else {
            perror("Error reading from socket");
        }

    }

    close(sockfd);
    pthread_exit(NULL);
}
int main() {
	pthread_t threads[MAX_THREADS];
	int i;
	struct sockaddr_in *clientAddress = malloc(sizeof(struct sockaddr_in));
	/*for (i = 0; i < MAX_THREADS; i++)*/ {
		if (pthread_create(&threads[0], NULL, threadFunc, clientAddress) != 0) {
            fprintf(stderr, "Error creating thread %d\n", 1);
            exit(1);
        }
        if (pthread_create(&threads[1], NULL, threadFunc2, clientAddress) != 0) {
            fprintf(stderr, "Error creating thread %d\n", 2);
            exit(1);
        }
	}

	// Wait for all threads to finish
    for (i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
