#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>

struct user {
	char nick[128];
	int socket;
	int connected;
};

#define MAX_USERS 128

struct user users[MAX_USERS];

int recv_socket = -1;

int server_fd, new_socket, valread, max_sd, max_client_sd;
struct sockaddr_in address;
int opt;
int addrlen;
char buffer[1024];
fd_set readfds, clientfds;

void listusers();
void send_everyone(char *nick, char *send_string);
void send_everyone_but(char *nick, char *send_string);

int adduser(char *nick, int socket)
{
	if (nick == NULL) {
		printf("no username specified, adduser() failed\n");
		return 1;
	}
	int added = 0;
	for (int i = 0; i < MAX_USERS; i++) {
		if (strlen(users[i].nick) == 0) {
			strcpy(users[i].nick, nick);
			users[i].socket = socket;
			users[i].connected = 1;
			added = 1;
			break;
		}
	}

	if (added) {
		char *m;
		asprintf(&m, "SERVER: %s tuli kahtelemaan mitä tiällä tapahtuu.", nick);
		printf("%s\n", m);
		send_everyone_but(nick, m);
		free(m);
	}

	return 0;
}

struct user *user_find_by_socket(int socket)
{
	for (int i = 0; i < MAX_USERS; i++) {
		if (users[i].socket == socket) {
			return &users[i];
		}
	}
	return NULL;
}

void removeuser(char *nick)
{
	char *m;
	asprintf(&m, "%s päätti lähtee tiältä.", nick);
	printf("%s\n", m);
	for (int i = 0; i < MAX_USERS; i++) {
		if (strcmp(users[i].nick, nick) == 0 && strlen(users[i].nick) > 0 && users[i].connected == 1) {
			users[i].connected = 0;
			memset(&users[i], 0, sizeof(struct user));
			send_everyone("", m);
			free(m);
		}
	}
}

int getsocketbynick(char *nick)
{
	for (int i = 0; i < MAX_USERS; i++) {
		if (strcmp(users[i].nick, nick) == 0) {
			return users[i].socket;
		}
	}
	return -1;
}

void listusers()
{
	printf("Listing all users:\n");
	for (int i = 0; i < MAX_USERS; i++) {
		if (strlen(users[i].nick) > 0) {
			printf("- <%s> (sock %d)\n", users[i].nick, users[i].socket);
		}
	}
	printf("________________\n");
}

char *listusers_string(char users_string[])
{
	strcpy(users_string, "");
	for (int i = 0; i < MAX_USERS; i++) {
		if (strlen(users[i].nick) > 0) {
			strcat(users_string, "[");
			strcat(users_string, users[i].nick);
			strcat(users_string, "] ");
		}
	}
	return users_string;
}

void init_users()
{
	for (int i = 0; i < MAX_USERS; i++) {
		memset(&users[i], 0, sizeof(struct user));
	}
}

void clear_users()
{
	for (int i = 0; i < MAX_USERS; i++) {
		memset(&users[i], 0, sizeof(struct user));
	}
}

void init_connection(int port)
{
	FD_ZERO(&readfds);
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
	FD_SET(server_fd, &readfds);
	max_sd = max_sd > server_fd ? max_sd : server_fd;
}

void send_everyone(char *nick, char *send_string)
{
	char tosend[65535] = {0};
	strcat(tosend, nick);
	strcat(tosend, "> ");
	strcat(tosend, send_string);
	strcat(tosend, "\n");

	for (int i = 0; i < MAX_USERS; i++) {
		send(users[i].socket ,  tosend, strlen(tosend) , 0);
	}
}

void send_everyone_but(char *nick, char *send_string)
{
	char tosend[65535] = {0};
	strcat(tosend, nick);
	strcat(tosend, "> ");
	strcat(tosend, send_string);
	strcat(tosend, "\n");

	for (int i = 0; i < MAX_USERS; i++) {
		if (strcmp(users[i].nick, nick) != 0) {
			send(users[i].socket ,  tosend, strlen(tosend) , 0);
		}
	}
}

int connections_handle(int sock)
{
	char *nick = NULL;
	char *msg_from_sock = NULL;
	char *msg_to_client = NULL;
	struct user *u = user_find_by_socket(sock);
	msg_from_sock = malloc(65535);
	memset(msg_from_sock, 0, 65535);
	read(sock, msg_from_sock, 65535);


	char token[65535] = {0};
	char command[65535] = {0};
	char parameter[65535] = {0};
	char cmd_[65535] = {0};
	int r;
	strcpy(cmd_, msg_from_sock);
	if (u != NULL) {

		if (strlen(msg_from_sock) < 1 && strlen(u->nick) > 0) {
			nick = malloc(strlen(u->nick) + 1);
			strcpy(nick, u->nick);
			removeuser(u->nick);
			free(nick);
			return 0;
		}

		if (msg_from_sock[0] == '/') {
			strcpy(token, strtok(cmd_, " "));
			strcpy(command, token);
			char *t = strtok(NULL, "\0");
			if (t != NULL) {
				strcpy(parameter, t);
			}

			if (strcmp(command, "/list") == 0) {
				char users_str[65535];
				listusers_string(users_str);
				r = asprintf(&msg_to_client, "SERVER: Palavelimella lie tämmösii käyttäjii: %s.\n", users_str);
				write(sock, msg_to_client, strlen(msg_to_client));
				free(msg_to_client);
				free(msg_from_sock);
				return 0;
			}

			if (strcmp(command, "/nick") == 0) {
				if (strlen(parameter) > 0) {
					if (getsocketbynick(parameter) == sock) {
						r = asprintf(&msg_to_client, "SERVER: Siepä oot jo sie :D elä yritä kusettaa!\n");
						write(sock, msg_to_client, strlen(msg_to_client));
					} else if (getsocketbynick(parameter) != -1) {
						r = asprintf(&msg_to_client, "SERVER: et sie sua toisten nimmii varastaa!\n");
						write(sock, msg_to_client, strlen(msg_to_client));
					} else {
						char oldnick[255];
						strcpy(oldnick, u->nick);
						strcpy(u->nick, parameter);
						r = asprintf(&msg_to_client, "SERVER: %s vaihto nimesä tämmöseks: %s.\n", oldnick, u->nick);
						send_everyone(u->nick, msg_to_client);
					}

				}

				free(msg_to_client);
				free(msg_from_sock);
				return 0;
			}


			if (strcmp(command, "/whois") == 0) {
				if (strlen(parameter) > 0) {
					int s = getsocketbynick(parameter);
					if (s < 0) {
						r = asprintf(&msg_to_client, "SERVER: Eipä oo vielä yhistäny tämä kaveri :/\n");
					} else {
						r = asprintf(&msg_to_client, "SERVER: Tiällä on %s ja sillä on sokettiloinen nrolla %d, jeejee :D\n", parameter, s);
					}
					write(sock, msg_to_client, strlen(msg_to_client));
				}

				free(msg_to_client);
				free(msg_from_sock);
				return 0;
			}
		}

		// normal message if no commands
		send_everyone(u->nick, msg_from_sock);

	} else {


		if (msg_from_sock[0] == '/') {
			strcpy(token, strtok(cmd_, " "));
			strcpy(command, token);
			char *t = strtok(NULL, "\0");
			if (t == NULL) {
				r = asprintf(&msg_to_client, "SERVER: mittees tiällä tapahtuu? t == NULL\n");
				write(sock, msg_to_client, strlen(msg_to_client));
				free(msg_to_client);
				free(msg_from_sock);
				return 0;
			}
			strcpy(parameter, t);
			if (strcmp(command, "/nick") == 0) {
				/* forward to server as is */
				nick = malloc(65535);
				memset(nick, 0, 65535);
				strcpy(nick, parameter);
				adduser(nick, sock);

				const char *welcome_msg = "SERVER: Ka päivvee, tulit sitte meiän servulle, %s! Meitä on täällä: %s.\n";
				char users_str[65535];
				listusers_string(users_str);
				r = asprintf(&msg_to_client, welcome_msg, parameter, users_str);
				write(sock, msg_to_client, strlen(msg_to_client));


				free(msg_to_client);
				free(msg_from_sock);
				free(nick);
				return 0;
			} else {
				r = asprintf(&msg_to_client, "SERVER: Kerroppa miulle nikki tälleesä: /nick <param>\n");
				write(sock, msg_to_client, strlen(msg_to_client));
				free(msg_to_client);
				free(msg_from_sock);
				return 0;
			}
		} else {
			r = asprintf(&msg_to_client, "SERVER: Kerroppa miulle nikki tälleesä: /nick <param>\n");
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
		printf("select error\n");
		return 1;
	}
	if (activity == 0) {
		return 0;
	}
	if (FD_ISSET(server_fd, &readfds)) {
		if ((new_socket = accept(server_fd,
		                         (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
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


	for (int i = 0; i < MAX_USERS; i++) {
		if (strlen(users[i].nick) > 0) {
			FD_SET(users[i].socket, &clientfds);
			max_client_sd = max_client_sd > users[i].socket ? max_client_sd : users[i].socket;
		} else {
			break;
		}
	}

	int activity = select( max_client_sd + 1 , &clientfds , NULL , NULL , &timeout);

	if ((activity < 0) && (errno != EINTR)) {
		return 1;
	}
	if (activity == 0) {
		return 0;
	}

	for (int i = 0; i < MAX_USERS; i++) {
		if (strlen(users[i].nick) > 0 && strcmp(users[i].nick, "") != 0) {
			if (FD_ISSET(users[i].socket, &clientfds)) {
				r |= connections_handle(users[i].socket);
			}

		} else {
			break;
		}
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