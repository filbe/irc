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

struct user {
	char *nick;
	int socket;
};

struct users {
	struct user *user;
	struct users *prev;
	struct users *next;
};

struct users *all_users;
struct users *last_user;

int recv_socket = -1;

void adduser(struct user _u)
{
	struct users *n = malloc(sizeof(struct users));
	struct user *u = malloc(sizeof(struct user));
	memcpy(u, &_u, sizeof(struct user));
	n->user = u;
	last_user->next = n;
	n->prev = last_user;
	last_user = n;
	printf("%s %d %s %d %d\n", u->nick, u->socket, _u.nick, _u.socket, all_users->user->socket);
}

void removeuser(struct user *u)
{
	struct users *current = all_users;
	while (current->user != NULL) {
		if (strcmp(u->nick, current->user->nick) == 0) {
			current->next->prev = current->prev;
			current->prev->next = current->next;
			free(u);
			break;
		}
		current = current->next;
	}
}

void listusers()
{
	struct users *current = all_users;
	printf("all users:\n");
	while (current->user != NULL) {
		printf("%s\n", current->user->nick);
		if  (current->next == NULL) {
			break;
		}
		current = current->next;
	}
}

void init_users()
{
	last_user = malloc(sizeof(struct users));
	last_user->user = malloc(sizeof(struct user));
	last_user->user->nick = "jotaki";
	all_users = last_user;
	printf("all_users->user->nick: %s\n", all_users->user->nick);
}

void clear_users()
{
	while (all_users) {
		struct users *next = all_users->next;
		free(all_users);
		all_users = next;
	}
}

int server_fd, new_socket, valread, max_sd;
struct sockaddr_in address;
int opt;
int addrlen;
char buffer[1024];
fd_set readfds;

void init_connection(int port)
{

	opt = 1;
	addrlen = sizeof(address);
	memset(&buffer, 0, sizeof(buffer));

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
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
	max_sd = server_fd;

	// Clear an fd_set
	FD_ZERO(&readfds);

	// Add a descriptor to an fd_set
	FD_SET(server_fd, &readfds);

}

void receive_sync(char ** received_msg)
{
	int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

	if ((activity < 0) && (errno != EINTR)) {
		printf("select error");
	}

	char *hello = "Hello from server";
	if (FD_ISSET(server_fd, &readfds)) {
		if ((new_socket = accept(server_fd,
		                         (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		valread = read( new_socket , buffer, 1024);
		char *delim = " ";
		char *word = strtok(buffer, delim);
		if (strcmp(word, "/nick") == 0) {
			word = strtok(NULL, delim);
			struct user new_user;
			new_user.nick = word;
			new_user.socket = new_socket;
			adduser(new_user);
			printf("nick set: %s\nsocket: %d\n", word, new_socket);
		} else {
			printf("paskaa\n");
		}
		printf("%s\n", buffer );
		send(new_socket , hello , strlen(hello) , 0 );
		printf("Hello message sent\n");
		listusers();
		return;
	}


}

int main()
{
	int port = 6667;

	init_users();
	init_connection(port);

	char **received_msg = NULL;
	while (1) {
		receive_sync(received_msg);
	}

	clear_users();
	return 0;
}