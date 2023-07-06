#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#define MAX_EVENTS 10

int main() {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }

    int socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    // Set up socket1 for listening on a specific UDP port
   if (socket1 < 0)
	{
		perror("socket1");
		return -1;
	}

	// Set the socket to non-blocking mode
	int flags1 = fcntl(socket1, F_GETFL, 0);
	fcntl(socket1, F_SETFL, flags1 | O_NONBLOCK);

	struct sockaddr_in address1;
	memset(&address1, 0, sizeof(address1));
	address1.sin_family = AF_INET;
	address1.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to all available network interfaces
	address1.sin_port = htons(12345);		 // Replace PORT_NUMBER with the desired port

	if (bind(socket1, (struct sockaddr *)&address1, sizeof(address1)) < 0)
	{
		perror("bind");
		close(socket1);
		return -1;
	}

 
    int socket2 = socket(AF_INET, SOCK_DGRAM, 0);
    // Set up socket2 for listening on a different UDP port
	if (socket2 < 0)
	{
		perror("socket2");
		return -1;
	}

	// Set the socket to non-blocking mode
	int flags2 = fcntl(socket2, F_GETFL, 0);
	fcntl(socket2, F_SETFL, flags2 | O_NONBLOCK);

	struct sockaddr_in address2;
	memset(&address2, 0, sizeof(address2));
	address2.sin_family = AF_INET;
	address2.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to all available network interfaces
	address2.sin_port = htons(8888);		 // Replace PORT_NUMBER with the desired port

	if (bind(socket2, (struct sockaddr *)&address2, sizeof(address2)) < 0)
	{
		perror("bind");
		close(socket2);
		return -1;
	}

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = socket1;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket1, &event) == -1) {
        perror("epoll_ctl socket1");
        return 1;
    }

    event.data.fd = socket2;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket2, &event) == -1) {
        perror("epoll_ctl socket2");
        return 1;
    }

    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == socket1) {
                // Handle data received on socket1
                // Read from socket1
            } else if (events[i].data.fd == socket2) {
                // Handle data received on socket2
                // Read from socket2
            }
        }
    }

    close(socket1);
    close(socket2);
    close(epoll_fd);

    return 0;
}

