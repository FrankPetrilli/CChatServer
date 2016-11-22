#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>

#define MAX_SIZE 512

int sock;
fd_set active_fd_set, read_fd_set;
fd_set chat_room_1, chat_room_2;

void signal_handler(int signal_value)
{
	if (signal_value == SIGINT)
	{
		close(sock);
		exit(EXIT_SUCCESS);
	}
}

void read_from_client(int fd)
{
	char *buffer = calloc(MAX_SIZE, sizeof(char));
	
	int nbytes = read(fd, buffer, MAX_SIZE);
	if (nbytes == 0)
	{
		close(fd);
		FD_CLR(fd, &active_fd_set);
	}
	else
	{
		if (FD_ISSET(fd, &chat_room_1))
		{
			for (int i = 0; i < FD_SETSIZE; i++)
			{
				if (FD_ISSET(i, &chat_room_1) && fd != i)
				{
					send(i, buffer, strlen(buffer), 0);
				}
			}
		}
		if (FD_ISSET(fd, &chat_room_2))
		{
			for (int i = 0; i < FD_SETSIZE; i++)
			{
				if (FD_ISSET(i, &chat_room_2) && fd != i)
				{
					send(i, buffer, strlen(buffer), 0);
				}
			}
		}
	}
	free(buffer);
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: %s portnum\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, signal_handler);
	struct sockaddr_in name;

	/* Create the socket. */
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	name.sin_family = AF_INET;
	name.sin_port = htons(atoi(argv[1]));
	name.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0)
	{
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	if (listen(sock, 1) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	// Accept clients
	
	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set); // Set the server as active
	
	for (;;)
	{
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
		{
			perror("select");
			exit(EXIT_FAILURE);
		}
		printf("Selected\n");
		for (int i = 0; i < FD_SETSIZE; i++)
		{
			if (FD_ISSET(i, &read_fd_set))
			{
				if (i == sock) // A connection on the master
				{
					printf("New client\n");
					struct sockaddr_in client_socket;
					int new_fd;
					socklen_t size = sizeof(client_socket);
					new_fd = accept(sock, (struct sockaddr *)&client_socket, &size);
					char* sendbuff = "Send a or b to join that room\n";
					char* recbuff = calloc(4, sizeof(char));
					send(new_fd, sendbuff, strlen(sendbuff), 0);
					read(new_fd, recbuff, 4);
					if (*recbuff == 'a')
					{
						char* sendbuff2 = "Joined a\n";
						send(new_fd, sendbuff2, strlen(sendbuff2), 0);

						FD_SET(new_fd, &chat_room_1);
					}
					else
					{
						char* sendbuff2 = "Joined b\n";
						send(new_fd, sendbuff2, strlen(sendbuff2), 0);
						FD_SET(new_fd, &chat_room_2);
					}
					free(recbuff);
					FD_SET(new_fd, &active_fd_set);
				}
				else
				{
					printf("Known client\n");
					read_from_client(i);
				}
			}
		}
	}
}
