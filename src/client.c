#define _GNU_SOURCE
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define MAXBUFSIZE 65535

struct servers {
	char **channels;
	int socket;
	struct servers *prev;
	struct servers *next;
};

char msgs[128][MAXBUFSIZE] = {0};

void drawMessages();

void addMessage(char *message)
{
	for (int i = 1; i < 128; i++) {
		memcpy(msgs[i - 1], msgs[i], MAXBUFSIZE);
	}
	memcpy(msgs[127], message, strlen(message) + 1);
	drawMessages();
}

void drawMessages()
{
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	int total_rows = w.ws_row - 5;

	printf("\e[1;1H\e[2J\033[0;0H");
	for (int i = 127 - total_rows; i < 128; i++) {
		printf("%s", msgs[i]);
	}
	printf("\033[%d;0H\n\n", total_rows + 3);
}

char nick[255];

int current_window_sock = -1;

// const char *c = "";
// int strlen(char *c) {
// 	int counter = 0;
// 	while(c[counter] != 0) {
// 		counter++;
// 	}
// 	return counter;
// }
int server_send(int sock, char *cmd);

char *concat(const char *str1, const char *str2)
{
	int sz = strlen(str1) + strlen(str2) + 1;
	char *ret = malloc(sz);
	memset(ret, 0, sz);
	strcpy(ret, str1);
	strcat(ret, str2);
	return ret;
}

void server_new(int sock)
{
	/* TODO: create new instance to servers linked list */
}

void server_connect(char *server, int portno)
{
	/* TODO: server connection */
	struct sockaddr_in serv_addr;
	serv_addr.sin_addr.s_addr = inet_addr(server);
	current_window_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (current_window_sock < 0) {
		printf("ERROR opening socket");
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	if (connect(current_window_sock, &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR connecting");
	} else {
		printf("socket: %d\n", current_window_sock);
		char *connect_msg;
		if (asprintf(&connect_msg, "/nick %s", nick)) {

		}

		server_send(current_window_sock, connect_msg);
		free(connect_msg);
	}


	// int sock = 0;

	// server_new(sock);
	// current_window_sock = sock;
}

int server_send(int sock, char *cmd)
{
	if (sock < 3) {
		printf("Socket unset!\n");
		return 1;
	}
	/* TODO: send stuff to server */
	if (strlen(cmd) > 0) {
		write(sock, cmd, strlen(cmd));
	}
	return 0;
}

void all_servers_send()
{
	/* TODO: send stuff to all servers */
}


char *server_read(int sock)
{
	char *r = malloc(65535);
	memset(r, 0, 65535);
	read(sock, r, 65535);
	return r;
}

void server_disconnect(int sock)
{
	/* TODO: send disconnect signal to server and close connection */
}



void nick_set(char *n)
{
	printf("nick_set(): %s\n", n);
	strcpy(nick, n);
	all_servers_send(concat("/nick ", nick));
}


char last_command[65535];
void command_get(char *cmd)
{
	char str[65535] = {0};
	int i = 0;
	char c;
	printf("%s> ", nick);
	do {
		c = getchar();
		if (c == '\n' || c == 0 || c == EOF) {
			str[i++] = 0;
			break;
		}
		str[i++] = c;
	} while (1);
	strcpy(cmd, str);
	if (strlen(cmd) == 0){
		command_get(cmd);
	}
	// strcpy(last_command, cmd);
}

int command_parse(char *cmd)
{
	char token[65535] = {0};
	char command[65535] = {0};
	char parameter[65535] = {0};
	char cmd_[65535] = {0};
	strcpy(cmd_, cmd);
	if (cmd[0] == '/') {
		strcpy(token, strtok(cmd_, " "));
		strcpy(command, token);
		char *t = strtok(NULL, "\0");
		if (t != NULL) {
			strcpy(parameter, t);
		}

		if (strcmp(command, "/msg") == 0 ||
		        strcmp(command, "/nick") == 0 ||
		        strcmp(command, "/join") == 0 ||
		        strcmp(command, "/whois") == 0 ||
		        strcmp(command, "/list") == 0) {
			/* forward to server as is */
			server_send(current_window_sock, cmd);
			if (strcmp(command, "/nick") == 0) {
				nick_set(strtok(parameter, " "));
			}
			return 0;
		} else if (strcmp(command, "/connect") == 0) {
			char *server;
			int port;
			server = malloc(strlen(parameter) + 1);
			strcpy(server, parameter);
			server = strtok(server, " ");
			port = atoi(strtok(NULL, "\0"));
			server_connect(server, port);
			return 0;
		} else {
			return 1;
		}
	} else {
		server_send(current_window_sock, cmd);
		return 0;
	}
	return 1; /* command failed */
}

int write_complete = 0;

void *writing_thread()
{
	write_complete = 0;
	memset(last_command, 0, sizeof(last_command));
	command_get(last_command);
	if (command_parse(last_command)) {
		printf("Command failed, invalid command or missing parameters! '%s'\n", last_command);
	}
	write_complete = 1;
	return 0;
}


int main(int argc, char *argv[])
{
	getlogin_r(nick, sizeof(nick));
	if (argc > 1 && strlen(argv[1]) > 0) {
		nick_set(argv[1]);
	}

	pthread_t w_t_id;
	pthread_create(&w_t_id, NULL, writing_thread, NULL);



	while (1) {
		if (write_complete == 1) {
			pthread_join(w_t_id, NULL);
			pthread_create(&w_t_id, NULL, writing_thread, NULL);
		}
		if (nick != NULL && strlen(nick)) {
			char *p = server_read(current_window_sock);
			if (strlen(p) > 0) {
				addMessage(p);
			}
			free(p);
		}
	}
	pthread_join(w_t_id, NULL);
}