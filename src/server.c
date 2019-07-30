#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>


struct users {
	char nick[128];
	int socket;
	struct users *prev;
	struct users *next;
};

struct users *all_users;
struct users *last_user;

int recv_socket = -1;

int server_fd, new_socket, valread, max_sd, max_client_sd;
struct sockaddr_in address;
int opt;
int addrlen;
char buffer[1024];
fd_set readfds, clientfds;

int adduser(char *nick, int socket)
{
	if (nick == NULL) {
		printf("no username specified, adduser() failed\n");
		return 1;
	} else {
		printf("adding user %s\n", nick);
	}

	max_sd = max_sd > socket ? max_sd : socket;
	if (strcmp(last_user->nick, "") == 0) {
		strcpy(last_user->nick, nick);
		last_user->socket = socket;
	} else {
		struct users *new_user = malloc(sizeof(struct users));
		strcpy(new_user->nick, nick);
		new_user->socket = socket;
		new_user->prev = last_user;
		last_user->next = new_user;
		last_user = new_user;
	}
	return 0;
}

struct users *user_find_by_socket(int socket)
{
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
		if (cur_u->socket == socket) {
			return cur_u;
		}
		cur_u = cur_u->next;
	}
	return NULL;
}

void removeuser(char *nick)
{
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
		if (strcmp(cur_u->nick, nick) == 0) {
			if (cur_u->prev != NULL) {
				cur_u->prev->next = cur_u->next;
			}
			if (cur_u->next != NULL) {
				cur_u->next->prev = cur_u->prev;
			}
			struct users *todel = cur_u;
			cur_u = cur_u->next;
			free(todel);
			return;
		}
		cur_u = cur_u->next;
	}
}

int getsocketbynick(char *nick)
{
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
		if (strcmp(cur_u->nick, nick) == 0) {
			return cur_u->socket;
		}
		cur_u = cur_u->next;
	}
	return 0;
}

void listusers()
{
	struct users *cur_u;
	cur_u = all_users;
	printf("Listing all users:\n");
	while (cur_u->nick) {
		printf("- <%s>\n", cur_u->nick);
		cur_u = cur_u->next;
	}
	printf("________________\n");
}

char *listusers_string(char users_string[])
{
	strcpy(users_string, "");
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
		strcat(users_string, "[");
		strcat(users_string, cur_u->nick);
		strcat(users_string, "] ");
		cur_u = cur_u->next;
	}
	return users_string;
}

void init_users()
{
	all_users = malloc(sizeof(struct users));
	last_user = all_users;
}

void clear_users()
{

}

void init_connection(int port)
{
	FD_ZERO(&readfds);
	opt = 1;
	memset(&buffer, 0, sizeof(buffer));

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		printf("ic1\n");
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
	               &opt, sizeof(opt))) {
		printf("ic2\n");
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( port );

	if (bind(server_fd, (struct sockaddr *)&address,
	         sizeof(address)) < 0) {
		printf("ic3\n");
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		printf("ic4\n");
		perror("listen");
		exit(EXIT_FAILURE);
	}
	addrlen = sizeof(address);
	FD_SET(server_fd, &readfds);
	max_sd = max_sd > server_fd ? max_sd : server_fd;
}

void send_everyone(char *nick, char *send_string)
{
	struct users *cur_u;
	cur_u = all_users;
	char tosend[65535] = {0};
	strcat(tosend, nick);
	strcat(tosend, "> ");
	strcat(tosend, send_string);
	strcat(tosend, "\n");
	while (cur_u->nick) {
		send(cur_u->socket ,  tosend, strlen(tosend) , 0);
		cur_u = cur_u->next;
	}
}

int connections_handle(int sock)
{
	char *nick = NULL;
	char *msg_from_sock = NULL;
	char *msg_to_client = NULL;
	struct users *u = user_find_by_socket(sock);
	msg_from_sock = malloc(65535);
	memset(msg_from_sock, 0, 65535);
	read(sock, msg_from_sock, 65535);
	if (u != NULL) {
		/*
		TODO:
		handle all commands that could be done.
		copy message from socket to msg_from_sock
		*/
		send_everyone(u->nick, msg_from_sock);

	} else {
		char token[65535] = {0};
		char command[65535] = {0};
		char parameter[65535] = {0};
		char cmd_[65535] = {0};
		int r;
		strcpy(cmd_, msg_from_sock);

		if (msg_from_sock[0] == '/') {
			strcpy(token, strtok(cmd_, " "));
			strcpy(command, token);
			char *t = strtok(NULL, "\0");
			if (t == NULL) {
				r = asprintf(&msg_to_client, "unknown error t == NULL");
				write(sock, msg_to_client, strlen(msg_to_client));
				free(msg_to_client);
				free(msg_from_sock);
				return 0;
			}
			strcpy(parameter, t);
			if (strcmp(command, "/nick") == 0) {
				/* forward to server as is */
				const char *welcome_msg = "Welcome to the most awesome irc ever, %s!";
				r = asprintf(&msg_to_client, welcome_msg, parameter);
				write(sock, msg_to_client, strlen(msg_to_client));

				nick = malloc(65535);
				memset(nick, 0, 65535);
				strcpy(nick, parameter);
				adduser(nick, sock);
				free(msg_to_client);
				free(msg_from_sock);
				free(nick);
				return 0;
			} else {
				r = asprintf(&msg_to_client, "please specify a nick name first! /nick <param>");
				write(sock, msg_to_client, strlen(msg_to_client));
				free(msg_to_client);
				free(msg_from_sock);
				return 0;
			}
		} else {
			r = asprintf(&msg_to_client, "please specify a nick name /nick <param>");
			write(sock, msg_to_client, strlen(msg_to_client));
			free(msg_to_client);
			free(msg_from_sock);
			return 0;
		}

		if (r == -666) {
			printf("plää plää nyt meni pieleen");
		}
	}
	return 0;
}

int connections_incoming_handle()
{
	int r = 0;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;

	FD_ZERO(&readfds);
	FD_SET(server_fd, &readfds);
	int activity = select( max_sd + 1 , &readfds , NULL , NULL , &timeout);
	if ((activity < 0) && (errno != EINTR)) {
		printf("cih1\n");
		printf("select error\n");
		return 1;
	}
	if (activity == 0) {
		return 0;
	}
	if (FD_ISSET(server_fd, &readfds)) {
		if ((new_socket = accept(server_fd,
		                         (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			printf("cih2\n");
			perror("accept");
			exit(EXIT_FAILURE);
		}

		r |= connections_handle(new_socket);
		FD_SET(new_socket, &readfds);
		max_sd = max_sd > new_socket ? max_sd : new_socket;

		return r;
	}
	return 0;
}

int connections_active_handle()
{
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	FD_ZERO(&clientfds);
	int r = 0;
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick && cur_u->nick != NULL && strcmp(cur_u->nick, "") != 0) {
		FD_SET(cur_u->socket, &clientfds);
		max_client_sd = max_client_sd > cur_u->socket ? max_client_sd : cur_u->socket;
		cur_u = cur_u->next;
	}
	int activity = select( max_client_sd + 1 , &clientfds , NULL , NULL , &timeout);
	if ((activity < 0) && (errno != EINTR)) {
		return 1;
	}
	if (activity == 0) {
		return 0;
	}
	cur_u = all_users;
	while (cur_u->nick && strcmp(cur_u->nick, "") != 0) {
		if (FD_ISSET(cur_u->socket, &clientfds)) {
			r |= connections_handle(cur_u->socket);
		}
		cur_u = cur_u->next;
	}
	return r;
}

void receive_sync()
{
	if (connections_incoming_handle()) {
		printf("connections_incoming_handle failed!\n");
	}

	if (connections_active_handle()) {
		printf("connections_active_handle failed!\n");
	}
}

int main()
{
	int port = 1234;

	init_users();
	init_connection(port);

	while (1) {
		receive_sync();
	}

	clear_users();
	return 0;
}