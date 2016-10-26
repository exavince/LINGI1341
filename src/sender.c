#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "packet_interface.c"
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER "::1" //address of the server
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to send data


/**************************************
*          variables globales         *
**************************************/
uint8_t window_max = 5;  //Taille de la fenetre
uint8_t window = 5;  //Dernière position libre dans le buffer
uint8_t ack_position = 5;
uint8_t seqnum = 0;   //Numero de séquence du dernier packet envoyer
uint8_t file_end = 0;
uint8_t slot_free = 1;
int sockfd;   //Socket de communication
char *retour; //Supprimer apres retour d'un ack
pkt_t **pkt_buffer;   //Buffer packet a envoyer
pkt_t **ack_buffer;   //Buffer de gestions des acks
char *filename;   //Nom du fichier de payload
char *host_name;  //Nom (adresse) de l'hote
uint16_t udp_port = 0;  //Port d'envoi
int test_stdin = 0;   //Test pour redirection sur le stdin
int test_file = 0;    //Test pour le nom de fichier
FILE *file;   //Fichier a lire
uint8_t end = 1;  //Fin de la boucle d'envoi


/**************************************
*          Renvoi d'une erreure       *
**************************************/
void error(char *s) {
    perror(s);
    exit(1);
}


/**************************************
*          Taille des buffers         *
**************************************/
/*
* Fonction qui retourne la taille d'une chaine de données
* char[] s : Buffer de taille inconnue terminant pas '\0'
* i : Taille du buffer ('\0' inclu)
*/
uint16_t count(char s[]) {
  uint16_t i = 0;
  while(s[i]!='\0') {
    ++i;
  }
  return i + 1;
}


/**************************************
*          Creation de packet         *
**************************************/
void pkt_create(pkt_t *pkt, ptypes_t type, uint16_t length, char *payload) {
  pkt_set_type(pkt, type);
  pkt_set_window(pkt, window);
  pkt_set_seqnum(pkt, seqnum);
  pkt_set_length(pkt, length);
  pkt_set_payload(pkt,payload, length);;
  uint32_t timestamp = time(NULL);
  pkt_set_timestamp(pkt, timestamp);
}


/**************************************
*      Initialisation du socket       *
**************************************/
int socket_create() {
  //Initialisation des variables
  struct addrinfo hints, *res;
  int erreur;

  //Creation du socket
  printf("Connection au socket\n");
  sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd == -1) {
    error("Unable to create the socket");
  }

  //Recuperation de l'adresse ip
  memset (&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  erreur = getaddrinfo(SERVER, NULL, &hints, &res);
  if(erreur != 0) {
    error("Unable to recup the address");
  }

  //Connection du socket
  struct sockaddr_in6 *add = (struct sockaddr_in6 *) res->ai_addr;
  add->sin6_port = htons(PORT);
  erreur = connect(sockfd, (struct sockaddr *) add, sizeof(*add));
  if(erreur < 0) {
    error("Unable to connect\n");
  }
}


/**************************************
*           Lecture de ficher         *
**************************************/
void reader(FILE *file) {
  //Initialisation des variables
  uint16_t taille;
  pkt_t *pkt;
  char *tmp = malloc(sizeof(char)*512);
  bzero(tmp, 512);

  //Lecture du fichier
  if (fgets(tmp, 512, file) == NULL) {
      file_end = 1;
      fclose(file);
  }
  else {
    taille = count(tmp);
    pkt = pkt_new();
    pkt_create(pkt, PTYPE_DATA, taille, tmp);
    pkt_buffer[window_max - window] = pkt;
    bzero(tmp, 512);
    window--;
    seqnum++;
  }
}


/**************************************
*          Buffer mouvement           *
**************************************/
void buffer_move(uint8_t nombre, pkt_t **buffer) {
  uint8_t i;
  uint8_t j;
  for (j=0; j <= nombre; j++) {
    for (i=0; i<window_max; i++) {
      if (i == (window_max - 1)) {
        buffer[i] = NULL;
      }
      else {
        buffer[i] = buffer[i+1];
      }
    }
  }
}


/**************************************
*           Search for ack            *
**************************************/
uint8_t ack_receive(pkt_t *ack) {
  uint32_t ack_seqnum = pkt_get_seqnum(ack);
  uint8_t boucle = 0;
  while (boucle < window_max) {
    if (pkt_get_seqnum(ack_buffer[boucle]) == ack_seqnum) {
      buffer_move(boucle, ack_buffer);
      ack_position = ack_position + boucle+1;
      return 1;
    }
    boucle++;
  }
  return 0;
}


/**************************************
*               Writter               *
**************************************/
void writter(pkt_t **pkt_buffer, int position) {
  if (file_end == 1 && ack_position == window_max && window == window_max) {
    //Encodage de la deconnection
    printf("deconnection\n");
    end = 1;
    pkt_t *pkt = pkt_new();
    char *buffer = malloc(sizeof(char)*12);
    size_t size = 12;
    pkt_set_window(pkt, window);
    pkt_set_length(pkt, 0);
    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_timestamp(pkt, time(NULL));
    pkt_set_seqnum(pkt, seqnum-1);
    pkt_encode(pkt, buffer, &size);

    //Envoie de la deconnection
    int erreur = write(sockfd, buffer, 12);
    if (erreur < 0) {
      error("ERROR writing to socket");
    }
  }
  else {
    //Encodage de la structure
    pkt_t *pkt = pkt_buffer[position];
    printf("Envoi de : %s\n", pkt_get_payload(pkt));
    ack_buffer[window_max - ack_position] = pkt;
    ack_position--;
    size_t size = 12 + (size_t) pkt_get_length(pkt);
    char *buffer = malloc(sizeof(char)*size);
    pkt_encode(pkt, buffer, &size);

    //Envoi des données sur le socket
    int erreur = write(sockfd, buffer, size);
    if (erreur < 0) {
      error("ERROR writing to socket");
    }
  }
}


/**************************************
*             Fonction main           *
**************************************/
int main(int argc, char *argv[]) {
  //Initialisation des variables
  int erreur, i;
  struct sockaddr_in6 address;
  int address_length = sizeof(address);
  pkt_t *pkt;
  size_t taille_encode;
  retour = malloc(sizeof(char)*37);
  pkt_buffer = malloc(sizeof(pkt_t)*window);
  ack_buffer = malloc(sizeof(pkt_t)*window);
  struct timeval * tv = (struct timeval * )malloc(sizeof(struct timeval));
  fd_set * readfds, * writefds;
	readfds = (fd_set *) malloc(sizeof(fd_set));
	writefds =  (fd_set *) malloc(sizeof(fd_set));
  i=0;
  while (i < window) {
    pkt_buffer[i] = NULL;
    i++;
  }
  filename = malloc(sizeof(char)*100);
  host_name = malloc(sizeof(char)*100);

  //Recuperation des argument
	if(argc > 1)
	{
		int i;
		while(i < argc-1 & strcmp(argv[i], "-f") != 0)
			i++;
		char * filename = argv[i+1];

		//Ouverture du fichier
		if ((file = fopen(filename, "r")) == NULL) {
			error("Impossible d'ouvrir le fichier");
		}

	}else
	{
		file = stdin;
	}

  //Création du socket et connection
  socket_create();

  //Lecture du fichier
  while (window > 0) {
    reader(file);
  }

  //Demande de connection
  writter(pkt_buffer, 0);
  window++;
  buffer_move(0, pkt_buffer);
  while (end == 1) {
    FD_ZERO(readfds);
    FD_SET(sockfd, readfds);
    FD_ZERO(writefds);
    FD_SET(sockfd, writefds);
    tv->tv_sec = 2;
  	tv->tv_usec = 0;
    erreur = select(sockfd+1, readfds, NULL, NULL, tv);
    if (erreur < 0) {
      error("Probleme avec le select");
    }
    else if (erreur == 0) {;
      writter(ack_buffer, 0);
    }
    else {
      if(FD_ISSET(sockfd, readfds)) {
        erreur = recvfrom(sockfd, retour, 37, 0, (struct sockaddr *) &address, &address_length);
        if (erreur == -1) {
          error("Probleme avec recvfrom()");
        }
        pkt = pkt_new();
        pkt_decode(retour, 37, pkt);
        slot_free = pkt_get_window(pkt);
        ack_receive(pkt);
        end = 0;
      }
    }
  }


  //boucle de lecture-ecriture
  i = 0;
  while (end == 0) {
    if ((window_max - window) < window_max  && file_end == 0) {
      reader(file);
    }
    FD_ZERO(readfds);
    FD_SET(sockfd, readfds);
    FD_SET(STDIN_FILENO, readfds);
    FD_ZERO(writefds);
    FD_SET(sockfd, writefds);
    tv->tv_sec = 2;
  	tv->tv_usec = 0;
    erreur = select(sockfd+1, readfds, NULL, NULL, tv);
    if (erreur < 0) {
      error("Probleme avec le select()");
    }
    else if (erreur == 0) {
      for (i=0; i<window_max; i++) {
        if (ack_buffer[i] != NULL) {
          if (pkt_get_timestamp(ack_buffer[i])+5 < time(NULL)){
            writter(ack_buffer, i);
            pkt_set_timestamp(ack_buffer[i], time(NULL));
          }
        }
      }
    }
    else {
      if (FD_ISSET(sockfd, readfds)) {
        erreur = recvfrom(sockfd, retour, 37, 0, (struct sockaddr *) &address, &address_length);
        if (erreur == -1) {
          error("Probleme avec recvfrom()");
        }
        pkt = pkt_new();
        pkt_decode(retour, 37, pkt);
        slot_free = pkt_get_window(pkt);
        ack_receive(pkt);
      }
    }
    if (slot_free > 0 && ack_position > 0) {
      writter(pkt_buffer, 0);
      window++;
      buffer_move(0, pkt_buffer);
    }
  }

  //fermeture du socket et fin
  free(readfds);
  free(writefds);
  close(sockfd);
  return 0;
}
