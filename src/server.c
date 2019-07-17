#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>
/*

Serverillä pidetään kirjaa irc-kanavien käyttäjistä ja niihin liittyvästä socketeista.
Serverillä on lisäksi pääsocket (recv_socket), jolla vastaanotetaan uusien käyttäjien saapuvat socketit.

TODO:
Tällä hetkellä socketin lukeminen jää odottamaan dataa niin pitkäksi aikaa kunnes dataa on saatavilla.
- muuta ohjelmaa siten, että socketin statuksen voi tarkastaa ilman, että jäädään odottamaan dataa
	- tämä mahdollistaa useiden sockettien statuksen tutkimisen peräkkäin ilman, että jokaisesta on saatavilla dataa heti

- muuta ohjelmaa siten, että uudelta käyttäjältä vaaditaan ensimmäisessä viestissä käyttäjätunnus jossakin formaatissa.
	- tarkastetaan, että käyttäjätunnus on vapaana. Jos ei ole, hylätään yhteydenottopyyntö
	asianmukaisella virhesanomalla (käyttäjätunnus varattu / käyttäjätunnusta ei annettu oikeassa formaatissa)
	- jos käyttäjätunnus on vapaana, luodaan uusi käyttäjä kyseisellä nickillä ja liitetään socket siihen

- käydään läpi kaikki tallennetut socketit ja tarkastetaan, onko niihin tullut dataa

- parsitaan käyttäjän lähettämä data (toteutetaan komennot ja viestittelymahdollisuus)

*/



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
	

	// Add a descriptor to an fd_set
	

}

void send_everyone_but(char *nick, char *send_string)
{
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
		if (strcmp(cur_u->nick, nick) != 0) {
			char tosend[65535];
			strcat(tosend, nick);
			strcat(tosend, "> ");
			strcat(tosend, send_string);
			strcat(tosend, "\n");
			send(cur_u->socket ,  tosend, strlen(tosend) , 0);
		}
		cur_u = cur_u->next;
	}
}


void receive_sync(char ** received_msg)
{
	FD_ZERO(&readfds);
	FD_SET(server_fd, &readfds);
	max_sd = server_fd;

	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
		FD_SET(cur_u->socket, &readfds);
		max_sd = max_sd > cur_u->socket ? max_sd : cur_u->socket;
		cur_u = cur_u->next;
	}

	char * welcome = "Welcome to the server, \033[1m%s\033[0m!\nUsers in the server: %s\n";
	char *welcomenick;
	char *invalid_nick = "\033[01;33mMessage from the server: \033[1;31mERROR! Please reconnect with \033[0m\033[1m/nick <nick>\033[0m\n";

	printf("waiting for activity...\n");
	int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
	printf("some activity received!\n");

	if ((activity < 0) && (errno != EINTR)) {
		printf("select error");
	}

	if (activity == 0) {
		return;
	}

	printf("checking master sock\n");
	if (FD_ISSET(server_fd, &readfds)) {
		printf("something in master sock, accepting incoming sock...\n");
		if ((new_socket = accept(server_fd,
		                         (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		printf("new sock from master sock accepted, reading shit...\n");
		valread = read( new_socket , buffer, 1024);
		printf("read ok\n");
		char *space = " ";
		char *linebreak = "\n";
		char *word = strtok(buffer, space);
		if (strcmp(word, "/nick") == 0) {
			word = strtok(NULL, space);
			word = strtok(word, linebreak);
			adduser(word, new_socket);
			printf("\033[1;32mNick \033[0m\033[1m%s\033[0m \033[1;32mconnected. Socket: \033[0m\033[1m%d\033[0m\n", word, new_socket);
			char u_str[4096];
			listusers_string(u_str);
			asprintf(&welcomenick, welcome, word, u_str);
			send(new_socket ,  welcomenick, strlen(welcomenick) , 0);
		} else {
			printf("\033[01;33mSomeone tried to connect without telling nick. Warned them about it.\n\033[0m");
			send(new_socket ,  invalid_nick, strlen(invalid_nick) , 0);
		}
		return;
	} else {
		printf("existing sock...\n");
		struct users *cur_u;
		cur_u = all_users;
		while (cur_u->nick) {
			if (FD_ISSET(cur_u->socket, &readfds)) {
				printf("reading...\n");
				if ((valread = read(cur_u->socket, buffer, 1024)) == 0) {
					printf("%s disconnected!\n", cur_u->nick);
					close(cur_u->socket);
					removeuser(cur_u->nick);
				} else {
					char *linebreak = "\n";
					char *msg = strtok(buffer, linebreak);

					printf("MSG from %s: %s\n", cur_u->nick, msg);
					send_everyone_but(cur_u->nick, msg);
					return;
				}

			}
			cur_u = cur_u->next;
		}
		printf("existing sock operations completed\n");
	}

}

int main()
{
	int port = 1234;

	init_users();
	init_connection(port);

	char **received_msg = NULL;
	while (1) {
		receive_sync(received_msg);
	}

	clear_users();
	return 0;
}