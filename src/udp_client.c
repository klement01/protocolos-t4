#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <udp_common.h>
#include <udp_client.h>

void *udp_client(void *cdptr) {
    int sock;
    struct sockaddr_in server;
    struct sockaddr_in client;
    char buffer[BUFF_SIZE];
    unsigned int echolen, clientlen;
    int received = 0;
    ClientData* clientData = (ClientData*)cdptr;


    /* Create the UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Die("Failed to create socket");
    }

    /* Construct the server sockaddr_in structure */
    memset(&server, 0, sizeof(server));                  /* Clear struct */
    server.sin_family = AF_INET;                         /* Internet/IP */
    server.sin_addr.s_addr = inet_addr(clientData->ip);  /* IP address */
    server.sin_port = htons(atoi(clientData->port));     /* server port */

    /* Client started. */
    puts("UDP client started");

    /* Send the word to the server */
    char msg[BUFF_SIZE];
    while (1) {
        puts("Digite a mensagem:");
        fgets(msg, BUFF_SIZE, stdin);
        echolen = strlen(msg);
        msg[echolen-1]='\0';
        echolen--;
        if (sendto(sock, msg, echolen, 0,
                (struct sockaddr *) &server,
                sizeof(server)) != echolen) {
            perror("Mismatch in number of sent bytes");
        }

        /* Receive the word back from the server */
        clientlen = sizeof(client);
        received = recvfrom(sock, buffer, BUFF_SIZE, 0,
                    (struct sockaddr *) &client,
                    &clientlen);
        buffer[received] = '\0';
        /* Check that client and server are using same socket */
        if (server.sin_addr.s_addr != client.sin_addr.s_addr) {
            perror("Received a packet from an unexpected server");
            fprintf(stderr, "Server: %ud\n", server.sin_addr.s_addr);
            fprintf(stderr, "Client: %ud\n", client.sin_addr.s_addr);
            Die("");
        }
        printf("Received: ");
        puts(buffer);
    }
}
