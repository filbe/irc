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

# irc
IRC server / Client demo project

Don't copy this project since it is just a test. Not compatible with anything, ever! So please go away :)



IRC Server & Client C:llä

- serverin vaatimukset:
	- komentoriviparametreja:
		-p, --port [port] 										portti (oletus 6667)
		-C, --connections 										maksimi yhteyksien määrä (oletus 8)
		-f, --file <file> 										tiedoston polku. Tiedosto tulostetaan jokaiselle clientille yhdistettäessä.

	- toiminnot ja ominaisuudet:
		- ottaa vastaan clientin lähettämiä irc-komentoja, joita ovat:
			komento 	alias
			/join 		/j
			/part 		/p
			/nick
			/msg
			/action 	/me
			/whois 		/w
			/topic 		/t

		- pitää kirjaa käyttäjistä, niiden tiedoista ja kanavista
			- listaa tarvittavat tietokentät suunnitelmaksi paperille
				Vinkkejä:
					- mitkä kokonaisuudet kannattaisi ryhmitellä structeiksi?
					- mitkä muuttujat kannattaa tallentaa pointtereina? entä pointterien pointtereina?

		- virhetilanteissa antaa virheilmoituksen mutta ei pasko muodostettuja yhteyksiä tai kaadu tms
			- miten testaat, että server ei voi kaatua?


- clientin vaatimukset:
	- komentoriviparametreja:
		- (pakollinen)
		-p, --port 												portti (oletus 6667)
		-n, --nick												nimimerkki (oletus: nykyinen tietokoneen käyttäjänimi)
		--channel="<#channel1>[ #channel2[ #channel3[...]]]"	kanava[t], joille liitytään

	- toiminnot ja ominaisuudet:
		- lähettää serverille komentoja ja vastaanottaa sekä käsittelee vastaukset
			- komentoja (joita ei mainittu server-puolessa), ovat:
				/set 											aseta yksittäinen asetus
				/save											tallenna asetukset (tämän voi tehdä viimeisenä)
				/alias 											tallenna alias. tätä pitää voida käyttää täysin vapaasti paitsi komennoille,
																niin myös vaikkapa lyhentämään viestiä;
																esim: /alias rib "ribale räiskis" -komennon jälkeen jos lähetät viestin:
																$> ei kyllä yhtään jaksaisi syödä ribsejä
																niin lopputuloksena lähetätkin:
																$> ei kyllä yhtään jaksaisi syödä ribale räiskissejä
				/aliases 										listaa kaikki käytössä olevat aliakset

			<ei komentoa>									lähetä normaali viesti nykyisen ikkunan kanavalle
			<ALT + [0...9]>									vaihda nykyistä ikkunaa. Yhdessä ikkunassa yksi kanava, ensimmäisessä ikkunassa
															serverin tulostamat kakkapasketit, aivan kuten esim Irssissä

		- käynnistyksen yhteydessä lue tiedostoon tallennetut aiemmat asetukset (.ini-tiedosto. tämän voi tehdä viimeisenä)


HUOMIOITAVAA KEHITYKSESSÄ:

- tekstin muotoilu:
	- server vastaa siitä, että viestit ja /actionit lähetetään kaikille clienteille VALMIIKSI MUOTOILTUNA.
	- client vastaa omien toimintojensa muotoilusta muilta osin
	- client muotoilee aliaksensa myös omaan näkymäänsä

- hyödynnettäviä tietorakenteita:
	- struct
	- linked list
		- jos elementtien lukumäärä tuntematon (vrt. array, jonka tarpeellista kokoa et tiedä etukäteen), voit tehdä linked listin
			- koska koodataan C:tä, linked list on hyvä osata luoda rakenteena itse structin ja pointterien avulla
				- ei valmiita makroja tai kirjastoja netistä, vaan tutustu rakenteeseen esim tästä:
					https://urlzs.com/gpWhh

- suunnittele projekti ennen yhtäkään koodiriviä
	- mitkä ovat ensimmäiseksi rakennettavia, helposti testattavia mukavia pikkupalikoita?
	- mitä functioita, rakenteita, testejä ym kannattaa tehdä ennen tietyn kokonaisuuden rakentamista?
	- mikä olisi helppo kirjasto toteuttaa TCP-yhteyskäytäntö?
		- tätä ennen voi jo tehdä todella paljon palasia valmiiksi(!)
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
FILE *welcomefile;
char welcometext[4096];

int fsize(FILE *fp)
{
	int prev = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, prev, SEEK_SET); //go back to where we were
	return sz;
}

void read_welcome()
{
	char line[1024];
	while (fgets(line, fsize(welcomefile), welcomefile)){
		strcat(welcometext, line);
	}
}

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
	char msg[4096] = "";
	strcat(msg, nick);
	strcat(msg, "> ");
	strcat(msg, send_string);
	strcat(msg, "\n");
	struct users *cur_u;
	cur_u = all_users;
	while (cur_u->nick) {
		if (strcmp(cur_u->nick, nick) != 0) {
			send(cur_u->socket ,  msg, strlen(msg) , 0);
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

	char *welcometext_filled;
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
			read_welcome(welcomefile);

			asprintf(&welcometext_filled, welcometext, word, u_str);
			send(new_socket, welcometext_filled, strlen(welcometext_filled), 0);
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

int main(int argc, char *argv[])
{
	if (argc > 3) {
		printf("Too many parameters!\n");
		return 0;
	}

	char *argument;
	char *connections;
	char *filename = "welcome.txt";

	welcomefile = fopen(filename, "r");

	for (int i = 1; i < argc; i++) {
		asprintf(&argument, "%s", argv[i]);
		// if (strcmp(argument, "-p") == 0 || strcmp(argument, "--port") == 0) {
		// 	free(argument);
		// 	asprintf(&argument, "%s", argv[++i]);
		// 	port = argument;
		// 	printf("port: %s\n", port);
		// }
		if (strcmp(argument, "-c") == 0 || strcmp(argument, "--connections") == 0) {
			free(argument);
			asprintf(&argument, "%s", argv[++i]);
			connections = argument;
			printf("connections: %s\n", connections);
		} else if (strcmp(argument, "-f") == 0 || strcmp(argument, "--file") == 0) {
			free(argument);
			asprintf(&argument, "%s", argv[++i]);
			filename = argument;
			printf("connections: %s\n", connections);
		} else if (strcmp(argument, "-p") != 0 || strcmp(argument, "--port") != 0 || strcmp(argument, "-c") != 0 || strcmp(argument, "--connections") != 0) {
			printf("Invalid command line argument(s)!\n");
			free(argument);
			return 0;
		}
		free(argument);
	}

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