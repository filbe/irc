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

int server_fd, new_socket, valread, max_sd;
struct sockaddr_in address;
int opt;
int addrlen;
char buffer[1024];
fd_set readfds;

void adduser(char *nick, int socket)
{
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
}

struct users *user_find_by_socket(int socket) {
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
	opt = 1;
	memset(&buffer, 0, sizeof(buffer));

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
	               &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( port );

	if (bind(server_fd, (struct sockaddr *)&address,
	         sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	addrlen = sizeof(address);
}

void send_everyone_but(char *nick, char *send_string)
{
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
			char tosend[65535];
			strcat(tosend, nick);
			strcat(tosend, "> ");
			strcat(tosend, send_string);
			strcat(tosend, "\n");
			send(cur_u->socket ,  tosend, strlen(tosend) , 0);
		cur_u = cur_u->next;
	}
}

int connections_handle(int sock) {
	char *nick = NULL;
	char *msg_from_sock = NULL;
	struct users *u = user_find_by_socket(sock);
	if (u != NULL) {
		// user found!

		/*
		TODO:

		handle all commands that could be done.

		copy message from socket to msg_from_sock


		*/
		send_everyone_but(u->nick, msg_from_sock);

	} else {
		// user not found, create one!
		/* TODO: 
		check if we have /nick [nick] coming
			- create user
			*/
			adduser(nick, sock);
			/*
		if not, then
			- send error message to client: "no nick set!" and disconnect
			- return 0;
		*/
	}
	return 0;
}

int connections_incoming_handle() {
	int r = 0;
	/* TODO:
	
	check if we have something in incoming sock (check server_fd status)
		- new connection?
			new_socket = (get new socket)
			*/
			r |= connections_handle(new_socket);
			/*
		- other situations
			print some debug data. how to detect connection close or other exceptions?

	*/
	return r;
}

int connections_active_handle() {
	int r = 0;
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
		if (0 /* check if something is coming from cur_u->socket */) {
			r |= connections_handle(cur_u->sock);
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