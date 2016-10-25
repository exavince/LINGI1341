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

#define BUFLEN 512  //Max length of buffer
#define PORT 8888


/**************************************
*           variables globales        *
**************************************/
int sockfd, client_length, erreur;  //Gestion  des sockets + erreur globale
struct sockaddr_in6 add_host, add_client; //Adresse du sender et du receiver
int host_length = sizeof(add_client); //Taille d'une adresse
pkt_t **pkt_buffer;
uint8_t window = 5;
uint8_t window_position = 0;
uint8_t seq_start = 0;
uint8_t seq_end = 0;
uint8_t connection = 0;
uint8_t end = 0;
FILE *file = NULL;


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
uint16_t count_buffer(char s[]) {
  uint16_t i = 0;
  while(s[i]!='\0') {
    ++i;
  }
  return i + 1;
}


/**************************************
*        Connection au socket         *
**************************************/
/*
*
*/
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
*            check_buffer             *
**************************************/
/*
* Verifie si le paquet recu est deja present dans le buffer
* Verifie si le paquet recu entre bien dans la fenetre de reception
* buf : Buffer des paquets recu
* pkt : paquet a verifier
* found : retourne 1 si paquet est deja dans le buffer 0 sinon
*/
uint8_t check_buffer(pkt_t **buf, pkt_t *pkt) {
  uint8_t found = 0;
  uint8_t boucle = 0;
  uint8_t seqnum = pkt_get_seqnum(pkt);
  if ((window_position-1) < 0) {
    return found;
  }
  for (boucle = 0; boucle <= (window_position-1) && found == 0; boucle++) {
    if (pkt_get_seqnum(buf[boucle]) == seqnum) {
      found = 1;
    }
  }
  return found;
}


/**************************************
*         Decodage de packet          *
**************************************/
/*
* Prend le buffer recu et la transforme en structure paquet
* buf : Donnees envoyees par le sender
* return : new qui est le nouveau paquet creer a partir du buffer
*/
pkt_t* decodage(char *buf) {
  size_t taille = 0;
  uint16_t mem_taille = 0;
  pkt_t *new = pkt_new();
  memcpy(&mem_taille, buf+2, 2);
  mem_taille = ntohs(mem_taille);
  taille = (size_t) mem_taille + 12;
  pkt_decode(buf, taille, new);
  if (pkt_get_seqnum(new) <= seq_end && pkt_get_seqnum(new) >= seq_start) {
    if (check_buffer(pkt_buffer, new) == 0) {
      pkt_buffer[window_position] = new;
      window_position++;
    }
  }
  return new;
}


/**************************************
*       Encodage d'acquittement       *
**************************************/
/*
* Encode l'acquitement a partir du paquet recu
* data : buffer qui contiendra le paquet
* pkt : paquet a encoder
*/
pkt_t* acquittement(char *data, pkt_t *pkt) {
  pkt_t *ack = pkt_new();
  pkt_set_payload(ack, "Données recues biatch", 22);
  pkt_set_type(ack, PTYPE_ACK);
  pkt_set_timestamp(ack, pkt_get_timestamp(pkt));
  pkt_set_crc(ack, pkt_get_crc(pkt));
  pkt_set_window(ack, pkt_get_window(pkt));
  pkt_set_seqnum(ack, pkt_get_seqnum(pkt));
  size_t taille = 34;
  pkt_encode(ack, data, &taille);
  return ack;
}


/**************************************
*             Fonction main           *
**************************************/
int main(void) {
  //Initialisation des variables
  file = fopen("serv.txt" , "w+");
  int count = 0;
  seq_end = window - 1;
  char *buffer = malloc(sizeof(char)*BUFLEN);
  pkt_t *pkt = pkt_new();
  pkt_t *ack = pkt_new();
  char *data = malloc(sizeof(char)*34);
  size_t taille = 0;
  uint16_t pkt_taille = 0;
	pkt_buffer = malloc(sizeof(pkt_t)*window);
  uint8_t i = 0;
  pkt_t *pkt2 = pkt_new();
  while (i < window) {
    pkt_buffer[i] = NULL;
    i++;
  }
	i = 0;

  //Creation du socket
  socket_create();

  //Reception des packets
  while(end == 0) {
    //Initialisation du buffer
    bzero(buffer, BUFLEN);
    printf("Server listenning for data :\n");
    fflush(stdout);

    //Reception du buffer
    if ((client_length = recvfrom(sockfd, buffer, BUFLEN, 0, (struct sockaddr *) &add_client, &host_length)) == -1) {
      error("Problem with data receive");
    }
    if (connection == 0) {
      erreur = connect(sockfd, (struct sockaddr *) &add_client, sizeof(add_client));
      if(erreur < 0) {
        error("Unable to connect\n");
      }
      connection = 1;
    }
    printf("Data : %s\n", buffer+8);

    //Decodage du buffer vers la structure
    pkt = decodage(buffer);
    if (pkt_get_type(pkt) == 0) {
      end = 1;
    }
    printf("Seqnum : %d\n", seq_start);

    //Encodage le l'acquittement
    ack = acquittement(data, pkt);

    //Envoie de l'acquittement
    erreur = write(sockfd, data, 37);
    if (erreur < 0) {
      error("ERROR writing to socket");
    }

    if (window_position == window) {
      int i = 0;
      window_position = 0;
      for (i=0; i < window; i++) {
        fwrite(pkt_get_payload(pkt_buffer[i]) , pkt_get_length(pkt_buffer[i]) , 1 , file);
        pkt_buffer[i] = NULL;
      }
      seq_end = seq_end + window;
      seq_start = seq_start + window;
    }
  }

  if (window_position-1 > 0) {
    int i = 0;
    for (i=0; i < window_position-1; i++) {
      fwrite(pkt_get_payload(pkt_buffer[i]) , pkt_get_length(pkt_buffer[i]) , 1 , file);
      pkt_buffer[i] = NULL;
    }
  }

  //Fermeture et fin
  fclose(file);
  close(sockfd);
  return 0;
}
