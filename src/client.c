#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
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

char nick[255];

int current_window_sock = 0;

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

void server_connect(char server[256])
{
	/* TODO: server connection */
	int sock = 0;

	server_new(sock);
	current_window_sock = sock;
}

void server_send(int sock, char *cmd)
{
	/* TODO: send stuff to server */
}

void all_servers_send()
{
	/* TODO: send stuff to all servers */
}

void server_disconnect(int sock)
{
	/* TODO: send disconnect signal to server and close connection */
}



void nick_set(char *n)
{
	strcpy(nick, n);
	all_servers_send(concat("/nick ", nick));
}


char *last_command = NULL;
void command_get(char *cmd)
{

	/* TODO: get command to cmd from user input */
	
	strcpy(cmd, "komento");
	sleep(1);
}

int command_parse(char *cmd)
{

	char *command = malloc(strlen(cmd)+1);
	char *parameter = malloc(strlen(cmd)+1);
	strcpy(command, cmd);
	/* TODO: parse /[command] [parameter] OR [msg] and then handle it properly */


	if (strcmp(command, "msg") == 0 ||
	        strcmp(command, "nick") == 0 ||
	        strcmp(command, "join") == 0 ||
	        strcmp(command, "whois") == 0 ||
	        strcmp(command, "") == 0) {
		/* forward to server as is */
		server_send(current_window_sock, cmd);
		return 0;
	} else if (strcmp(command, "connect") == 0) {
		server_connect(parameter);

		return 0;
	}

	return 1; /* command failed */
}


int main(int argc, char *argv[])
{
	last_command = malloc(10);
	strcpy(last_command, "/ripuli");
	while (1) {
		command_get(last_command);
		if (command_parse(last_command)) {
			printf("Command failed, invalid command! '%s'\n", last_command);
		}
	}
}