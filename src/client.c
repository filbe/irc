#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define FLUSH_STDIN(x) {if(x[strlen(x)-1]!='\n'){do fgets(Junk,16,stdin);while(Junk[strlen(Junk)-1]!='\n');}else x[strlen(x)-1]='\0';}
char Junk[16];

#define MAXBUFSIZE 65535
// #define PORT 1234
#define SA struct sockaddr

fd_set readfds;

char *nick = "";

int sockfd;

void *recv_server()
{
	while (1) {
		char recv_buff[MAXBUFSIZE];
		bzero(recv_buff, sizeof(recv_buff));
		read(sockfd, recv_buff, sizeof(recv_buff));
		//printf("%s\n", recv_buff);
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
	}
}

int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr_in servaddr;
	char *argument = "";
	int port = 0;

	char channels[40];

	for (int i = 1; i < argc; i++) {
		asprintf(&argument, "%s", argv[i]);
		if (strcmp(argument, "-p") == 0 || strcmp(argument, "--port") == 0) {
			free(argument);
			asprintf(&argument, "%s", argv[++i]);
			port = atoi(argument);
			printf("port: %d\n", port);
		} else if (strcmp(argument, "-n") == 0 || strcmp(argument, "--nick") == 0) {
			free(argument);
			asprintf(&argument, "%s", argv[++i]);
			nick = argument;
			printf("nickname: %s\n", nick);
		} else if (strcmp(argument, "-c") == 0 || strcmp(argument, "--channels") == 0) {
			free(argument);
			asprintf(&argument, "%s", argv[++i]);
			channels[i - 1] = *argument;
			printf("port: %s\n", channels);
		} else if (strcmp(argument, "-p") != 0 || strcmp(argument, "--port") != 0 || strcmp(argument, "-n") != 0 || strcmp(argument, "--nick") != 0
		           || strcmp(argument, "-c") != 0 || strcmp(argument, "-channel") != 0) {
			printf("Invalid command line argument(s)!\n");
			free(argument);
			return 0;
		}

		// socket create and varification
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == -1) {
			printf("socket creation failed...\n");
			exit(0);
		} else
			printf("Socket successfully created..\n");
		bzero(&servaddr, sizeof(servaddr));

		// assign IP, PORT
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		servaddr.sin_port = htons(port);

		// connect the client socket to server socket
		if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
			printf("connection with the server failed...\n");
			exit(0);
		} else {
			printf("connected to the server..\n");
		}

		pthread_t t_recv;

		pthread_create(&t_recv, NULL, recv_server, NULL);
		char send_buff[MAXBUFSIZE];
		while (1) {


			bzero(send_buff, MAXBUFSIZE * sizeof(char));



			while (1) {

				printf("Command> ");
				memset(&send_buff, 0, sizeof(send_buff));
				int bufcur = 0;
				while (1) {
					int c = getchar();

					if (c == -1 || c == 0 || c == EOF || c == '\n' || c == '\r') {
						send_buff[bufcur] = EOF;
						break;
					} else {
						send_buff[bufcur] = c;
					}
					bufcur++;
				}



				write(sockfd, send_buff, strlen(send_buff) + 1);
				printf("Str sent: %s\n", send_buff);

				bufcur = 0;
			}

		}
		close(sockfd);

	}
}
