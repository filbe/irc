#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
// #include <conio.h>
#define MAX 65535
// #define PORT 1234
#define SA struct sockaddr

fd_set readfds;

struct termios orig_termios;

void reset_terminal_mode()
{
	tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
	struct termios new_termios;

	/* take two copies - one for now, one for later */
	tcgetattr(0, &orig_termios);
	memcpy(&new_termios, &orig_termios, sizeof(new_termios));

	/* register cleanup handler, and set the new terminal mode */
	atexit(reset_terminal_mode);
	cfmakeraw(&new_termios);
	tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
	int r;
	unsigned char c;
	if ((r = read(0, &c, sizeof(c))) < 0) {
		return r;
	} else {
		return c;
	}
}

void func(int sockfd)
{
	char buff[MAX];
	for (;;) {
		bzero(buff, sizeof(buff));
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		printf("Enter the string : ");
		set_conio_terminal_mode();
		while (!kbhit()) {
			for (int i = 0; i < strlen(buff); i++) {
				if (buff[i] == '\n') {
					buff[i] = 0;
					write(sockfd, buff, i + 1);
				}
			}
			(void)getch(); /* consume the character */
			printf("%s", buff);

		}


		if (FD_ISSET(sockfd, &readfds)) {
			bzero(buff, sizeof(buff));
			read(sockfd, buff, sizeof(buff));
			printf("From Server : %s\n", buff);
		}
	}

	if ((strncmp(buff, "exit", 4)) == 0) {
		printf("Client Exit...\n");
		return 0;
	}
}


int main(int argc, char *argv[])
{
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;
	char *argument = "";
	int port = 0;
	char *nick = "";
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
			channels[i - 1] = argument;
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
		} else
			printf("connected to the server..\n");

		// function for chat
		func(sockfd);

		// close the socket
		close(sockfd);
	}
}