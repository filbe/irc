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

void adduser(char *nick, int socket)
{
	struct users *new_user = malloc(sizeof(struct users));
	strcpy(new_user->nick, nick);
	new_user->socket = socket;
	new_user->prev = last_user;
	last_user->next = new_user;
	last_user = new_user;

}

void removeuser(char *nick)
{

}

void listusers()
{
	struct users *cur_u;
	cur_u = all_users;
	printf("Listing all users:\n");
	while (cur_u->nick) {
		printf("- %s\n", cur_u->nick);
		cur_u = cur_u->next;
	}
	printf("________________\n");
}

void init_users()
{
	all_users = malloc(sizeof(struct users));
	last_user = all_users;
}

void clear_users()
{

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
			adduser(word, new_socket);

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