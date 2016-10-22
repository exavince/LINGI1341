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

#define BUFLEN 512  //Max length of buffer
#define PORT 8888


/**************************************
*           variables globales        *
**************************************/
int sockfd, client_length, erreur;  //Gestion  des sockets + erreur globale
struct sockaddr_in6 add_host, add_client; //Adresse du sender et du receiver
int host_length = sizeof(add_client); //Taille d'une adresse
pkt_t **pkt_buffer;
uint8_t window = 31;


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
uint16_t count_buffer(char s[]) {
  uint16_t i = 0;
  while(s[i]!='\0') {
    ++i;
  }
  return i;
}


/**************************************
*        Connection au socket         *
**************************************/
int socket_create() {
  if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
      error("Unable to create socket");
  }
  memset((char *) &add_host, 0, sizeof(add_host));
  add_host.sin6_family = AF_INET6;
  add_host.sin6_port = htons(PORT);
  add_host.sin6_addr = in6addr_any;
  if( bind(sockfd, (struct sockaddr*)&add_host, sizeof(add_host) ) == -1) {
      error("Unable to bind");
  }
}

/**************************************
*         Decodage de packet          *
**************************************/



/**************************************
*             Fonction main           *
**************************************/
int main(void) {
  //Initialisation des variables
  int count = 0;
  char *buffer = malloc(sizeof(char)*BUFLEN);
  pkt_t *pkt2 = pkt_new();
  char *data = malloc(sizeof(char)*27);
  size_t taille = 0;
  uint16_t pkt_taille = 0;
	pkt_buffer = malloc(sizeof(pkt_t));
	uint8_t i = 0;

  //Creation du socket
  socket_create();

  //Reception des packets
  while(i < window) {
    //Initialisation du buffer
    bzero(buffer, BUFLEN);
    printf("Server listenning for data :\n");
    fflush(stdout);

    //Reception du buffer
    if ((client_length = recvfrom(sockfd, buffer, BUFLEN, 0, (struct sockaddr *) &add_client, &host_length)) == -1) {
      error("Problem with data receive");
    }

    //Decodage vers la structure pkt
    memcpy(&pkt_taille, buffer+1, 1);
    pkt_taille = ntohs(pkt_taille);
    taille = (size_t) pkt_taille + 12;
    pkt_decode(buffer, taille, pkt2);
		pkt_buffer[i] = pkt2;

    //Encodage le l'acquittement
    printf("Data: %s\n" , pkt_get_payload(pkt2));
    printf("%d\n", pkt_get_seqnum(pkt2));
    pkt_set_payload(pkt2, "Données recues biatch", 22);
    pkt_set_type(pkt2, PTYPE_ACK);
    char *data_send = malloc(sizeof(char)*(34));
    taille = 34;
    pkt_encode(pkt2, data_send, &taille);

    //Envoie de l'acquittement
    if (sendto(sockfd, data_send, 37, 0, (struct sockaddr*) &add_client, host_length) == -1) {
      error("sendto()");
    }
		i++;
  }

  //Fermeture et fin
  close(sockfd);
  return 0;
}
