#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main()
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("socket");
		return -1;
	}

	// Set the socket to non-blocking mode
	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to all available network interfaces
	address.sin_port = htons(12345);		 // Replace PORT_NUMBER with the desired port

	if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind");
		close(sockfd);
		return -1;
	}
	int epollfd = epoll_create1(0);
	if (epollfd < 0)
	{
		perror("epoll_create1");
		close(sockfd);
		return -1;
	}

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = sockfd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) < 0)
	{
		perror("epoll_ctl");
		close(epollfd);
		close(sockfd);
		return -1;
	}

#define MAX_EVENTS 10
	struct epoll_event events[MAX_EVENTS];

	while (1)
	{
		int num_events = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (num_events < 0)
		{
			perror("epoll_wait");
			break;
		}
		fprintf(stderr, "epoll event:%d",num_events);
		for (int i = 0; i < num_events; i++)
		{
			if (events[i].data.fd == sockfd)
			{
				// Handle incoming UDP packet
				char buffer[1024];
				struct sockaddr_in sender_addr;
				socklen_t sender_len = sizeof(sender_addr);
				ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
											(struct sockaddr *)&sender_addr, &sender_len);
				fprintf(stderr, "recv len:%lu",recv_len);

				if (recv_len < 0)
				{
					if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						perror("recvfrom");
						break;
					}
				}
				else
				{
					// Process received packet
					// ...
					// You can access the received data in 'buffer'
					// 'recv_len' holds the number of bytes received
				}
			}
		}
	}

	return 0;
}
