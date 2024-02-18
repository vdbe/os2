/**
 * @author {AUTHOR}
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "lib/tcpsock.h"

#define PORT 5678
#define MAX_CONN 3  // state the max. number of connections the server will handle before exiting


/**
 * Implements a sequential test server (only one connection at the same time)
 */
int main(void) {
    tcpsock_t *server, *client;
    char data[BUFFER_MAX];
    int bytes, result;
    int conn_counter = 0;

    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    do {
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        printf("Incoming client connection\n");
        conn_counter++;
        do {
            // read data
            bytes = BUFFER_MAX;
            result = tcp_receive(client, (void *) &data, &bytes);
            if ((result == TCP_NO_ERROR) && bytes) {
                printf("Received : %s\n", data);
                bytes = strlen(data);
                bytes = bytes > BUFFER_MAX ? BUFFER_MAX : bytes;
                result = tcp_send(client,(void *) &data, &bytes);
            }
        } while (result == TCP_NO_ERROR);
        if (result == TCP_CONNECTION_CLOSED)
            printf("Peer has closed connection\n");
        else
            printf("Error occured on connection to peer\n");
        tcp_close(&client);
    } while (conn_counter < MAX_CONN);
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Test server is shutting down\n");
    return 0;
}




