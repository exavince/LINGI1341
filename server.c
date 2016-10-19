#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFLEN 512  //Max length of buffer
#define PORT 8888

void error(char *s) {
    perror(s);
    exit(1);
}

int main(void) {
    struct sockaddr_in6 add_host, add_client;
    int sockfd, client_length;
    int host_length = sizeof(add_client);
    char buffer[BUFLEN];

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

    while(1)
    {
        printf("Server listenning for data :\n");
        fflush(stdout);

        if ((client_length = recvfrom(sockfd, buffer, BUFLEN, 0, (struct sockaddr *) &add_client, &host_length)) == -1)
        {
            error("Problem with data receive");
        }
        printf("Data: %s\n" , buffer);

        if (sendto(sockfd, buffer, client_length, 0, (struct sockaddr*) &add_client, host_length) == -1)
        {
            error("sendto()");
        }
    }

    close(sockfd);
    return 0;
}
