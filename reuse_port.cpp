#include <iostream>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512
#define PORT 8888

void receive_data(int sockfd, int threadId)
{
    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);
    char buf[BUFLEN];

    while (true) {
        memset(buf, '\0', BUFLEN);

        int recv_len = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen);

        if (recv_len == -1) {
            std::cerr << "recvfrom() failed" << std::endl;
            continue;
        }

        std::cout << "thread:" << threadId << " Received packet from " << inet_ntoa(si_other.sin_addr) << ":" << ntohs(si_other.sin_port) << std::endl;
        std::cout << "Data: " << buf << std::endl;
    }
}

int main()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1) {
        std::cerr << "socket() failed" << std::endl;
        return 1;
    }

    // int optval = 1;
    // if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
    //     std::cerr << "setsockopt() failed" << std::endl;
    //     return 1;
    // }

    struct sockaddr_in si_me;
    memset((char *)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&si_me, sizeof(si_me)) == -1) {
        std::cerr << "bind() failed" << std::endl;
        return 1;
    }

    std::thread t1(receive_data, sockfd, 1);
    std::thread t2(receive_data, sockfd, 2);
    std::thread t3(receive_data, sockfd, 3);

    t1.join();
    t2.join();
    t3.join();

    close(sockfd);
    return 0;
}

