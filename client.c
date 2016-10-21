#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include "packet_interface.c"

#define SERVER "::1" //address of the server
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to send data


/**************************************
*          variables globales         *
**************************************/
uint8_t window = 31;  //Taille de la fenetre
uint8_t window_position = 0;  //Dernière position libre dans le buffer
uint8_t ack_position = 0;
uint8_t seqnum = 0;   //Numero de séquence du dernier packet envoyer
uint8_t file_end = 0;
int sockfd;   //Socket de communication
char *retour; //Supprimer apres retour d'un ack
pkt_t **ack_buffer;   //Buffer des ack recu
pkt_t **pkt_buffer;   //Buffer packet a envoyer
char *filename;   //Nom du fichier de payload
char *host_name;  //Nom (adresse) de l'hote
uint16_t udp_port = 0;  //Port d'envoi
int test_stdin = 0;   //Test pour redirection sur le stdin
int test_file = 0;    //Test pour le nom de fichier
FILE *file;   //Fichier a gerer


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
uint16_t count(char s[]) {
  uint16_t i = 0;
  while(s[i]!='\0') {
    ++i;
  }
  printf("composant %d\n", i);
  return i ;
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
void reader() {
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
    pkt_create(pkt, 1, taille, tmp);
    pkt_buffer[window_position] = pkt;
    ack_buffer[ack_position] = pkt;
    bzero(tmp, 512);
    window_position++;
    ack_position++;
    seqnum++;
  }
}


/**************************************
*            Gestion buffer           *
**************************************/
void buffer_move() {
  int i = 0;
  pkt_del(pkt_buffer[0]);
  while (pkt_buffer[i] != NULL && i < window) {
    pkt_buffer[i] = pkt_buffer[i+1];
    i++;
  }
  window_position--;
  if(file_end == 0) {
    reader();
  }
}


/**************************************
*           Search for ack            *
**************************************/
void ack_receive(pkt_t *ack) {
  uint32_t ack_timestamp = pkt_get_timestamp(ack);
  uint8_t boucle = 0;
  uint8_t end = 0;
  while (boucle < window && end == 0) {
    if (pkt_get_timestamp(ack_buffer[boucle]) == ack_timestamp) {
      end = 1;
      ack_buffer[boucle] == NULL;
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
  ack_buffer = malloc(sizeof(pkt_t)*(window));
  i=0;
  while (i < window) {
    pkt_buffer[i] = NULL;
    i++;
  }
  filename = malloc(sizeof(char)*100);
  host_name = malloc(sizeof(char)*100);

  //Recuperation des arguments
  for (i=1; i<argc; i++) {
    if(strcmp(argv[i], "-f") == 0) {
      strcpy(filename, argv[i+1]);
      i++;
      test_file = 1;
    }
    else if (strcmp(argv[i], "<") == 0) {
      strcpy(filename, argv[i+1]);
      i++;
      test_stdin = 1;
    }
    else {
      strcpy(host_name, argv[i]);
      i++;
      udp_port = atoi(argv[i]);
    }
  }

  //Création du socket et connection
  socket_create();

  //Ouverture du fichier
  if ((file = fopen("test.txt", "r")) == NULL) {
    error("Impossible d'ouvrir le fichier");
  }

  //Lecture du fichier
  while (file_end == 0 && window_position < window) {
    reader();
  }


  while (pkt_buffer[0] != NULL) {
    pkt = pkt_buffer[0];
    taille_encode = 12 + pkt_get_length(pkt);
    char *buf2 = malloc(sizeof(char)*taille_encode);
    pkt_encode(pkt, buf2, &taille_encode);

    //Envoi des données sur le socket
    erreur = write(sockfd,buf2, taille_encode);
    if (erreur < 0) {
      error("ERROR writing to socket");
    }
    buffer_move();

    //Reception de l'acknowlogement
    if (recvfrom(sockfd, retour, 37, 0, (struct sockaddr *) &address, &address_length) == -1) {
      error("recvfrom()");
    }
    pkt_t *receive;
    receive = pkt_new();
    pkt_decode (retour, 37, receive);
    puts(pkt_get_payload(receive));
    ack_receive(receive);
  }

  //fermeture du socket et fin
  close(sockfd);
  return 0;
}
