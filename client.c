#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER "::1"
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to send data

void error(char *s) {
    perror(s);
    exit(1);
}

int main(void) {
    struct sockaddr_in6 address;
    struct addrinfo hints, *res;
    int sockfd, erreur;
    int address_length = sizeof(address);

    if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        error("Unable to create the socket");
    }

    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    erreur = getaddrinfo(SERVER, NULL, &hints, &res);
    if(erreur!=0) {
    	error("Unable to recup the address");
  	}
    struct sockaddr_in6 *add = (struct sockaddr_in6 *) res->ai_addr;
    add->sin6_port = htons(PORT);
    if(connect(sockfd, (struct sockaddr *) add, sizeof(*add)) < 0) {
      fprintf(stderr,"Unable to connect\n");
      exit(0);
    }

    char *buffer = malloc(sizeof(char)*256);
    while(1) {
        printf("Enter message : ");
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        erreur = write(sockfd,buffer,strlen(buffer));
        if (erreur < 0) {
          error("ERROR writing to socket");
        }

        bzero(buffer,256);
        if (recvfrom(sockfd, buffer, BUFLEN, 0, (struct sockaddr *) &address, &address_length) == -1) {
            error("recvfrom()");
        }
        puts(buffer);
    }

    close(sockfd);
    return 0;
}
