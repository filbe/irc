#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct user {
	char *nick;
	// socket tähän;
};

struct users {
	struct users *prev;
	struct users *next;
	struct user *user;
};

struct users *all_users;
struct users *last_user;

int main (int argc, char *argv[])
{
	all_users = malloc(sizeof(all_users));
	last_user = all_users;
	struct users *u;
	u = malloc(sizeof(u));

	char *port;
	char *connections;

	u->next = NULL;
	u->prev = last_user;
	last_user->next = u;
	last_user = u;

	char *argument;
	
	for (int i = 1; i < argc; i++){
		asprintf(&argument, "%s", argv[i]);
		if (strcmp(argument, "-p") == 0 || strcmp(argument,"--port") == 0){
			free(argument);
			asprintf(&argument, "%s", argv[++i]);
			port = argument;
			printf("port: %s\n", port);
		} else if (strcmp(argument, "-c") == 0 || strcmp(argument, "--connections") == 0) {
			free(argument);
			asprintf(&argument, "%s", argv[++i]);
			connections = argument;
			printf("connections: %s\n", connections);	
		} else if (strcmp(argument, "-p") != 0 || strcmp(argument, "port") != 0 || strcmp(argument, "-c") != 0 || strcmp(argument, "connections") != 0){
			printf("Invalid command line argument(s)");
		}
		free(argument);
	}

	free(u);
	return 0;
}