#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

void handleClient(int socket) {
    // Handle incoming data from the client
    // ...
}

bool once = true;
int main() {
    const int PORT = 8888;
    const int MAX_THREADS = 2;

    // Create an array of UDP sockets
    std::vector<int> serverSockets(MAX_THREADS);

    // Create UDP sockets and set the SO_REUSEPORT socket option
    for (int i = 0; i < 1; ++i) {
        serverSockets[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (serverSockets[i] < 0) {
            std::cerr << "Failed to create socket." << std::endl;
            return 1;
        }

        int optval = 1;
        if (setsockopt(serverSockets[i], SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
            std::cerr << "Failed to set SO_REUSEPORT option." << std::endl;
            return 1;
        }

        // Bind each socket to the same address and port
        struct sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(PORT);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSockets[i], (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            std::cerr << "Failed to bind socket to port." << std::endl;
            return 1;
        }
    }

    // Start the worker threads
    std::vector<std::thread> threads;
	struct sockaddr_in *sharedAddres = nullptr;
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        threads.emplace_back([&serverSockets, &sharedAddres, i]()
                             {
            int serverSocket = serverSockets[i];
            if (i == 1)
            {
                while (once && !sharedAddres)
                {
                    /* code */
                }
                serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
                if (serverSocket < 0)
                {
                    std::cerr << "Failed to create socket." << std::endl;
                    return 1;
                }

                int optval = 1;
                if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) < 0)
                {
                    std::cerr << "Failed to set SO_REUSEPORT option." << std::endl;
                    return 1;
                }

                // Bind each socket to the same address and port
                struct sockaddr_in serverAddress
                {
                };
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(PORT);
                serverAddress.sin_addr.s_addr = INADDR_ANY;

                if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
                {
                    std::cerr << "Failed to bind socket to port." << std::endl;
                    return 1;
                }
                int ret = connect(serverSocket, (struct sockaddr *)sharedAddres, sizeof(*sharedAddres));
                if (ret < 0)
                {
                    perror("Error connection");
                    // return NULL;
                }
            }

            while (true) {
                struct sockaddr_in clientAddress{};
                socklen_t clientAddressLength = sizeof(clientAddress);
                char buffer[1024];

                // Receive data from a client
                ssize_t bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer), 0,
                                             (struct sockaddr*)&clientAddress, &clientAddressLength);

        // Get client IP address and port
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddress.sin_port);
        std::cout << "thread:" << i << " Received len:" << bytesRead << " from:" << clientIP << ":" << clientPort << std::endl;
        if (bytesRead < 0)
        {
            std::cerr << "Failed to receive data from client." << std::endl;
            continue;
        }
        if (once && i == 0)
        {
            sharedAddres = (sockaddr_in *)std::malloc(sizeof(struct sockaddr_in));
            std::memcpy(sharedAddres, &clientAddress, clientAddressLength);
            once = false;
        }

                // Handle the client in a separate thread
                handleClient(serverSocket);

                // Send a response to the client if needed
                // ...
            } });
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Close all server sockets
    for (int i = 0; i < MAX_THREADS; ++i) {
        close(serverSockets[i]);
    }

    return 0;
}

