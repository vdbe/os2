/**
 * @author Luc Vandeurzen
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "lib/tcpsock.h"

// conditional compilation option to control the number of measurements this sensor node wil generate
#if (LOOPS > 1)
#define UPDATE(i) (i--)
#else
#define LOOPS 1
#define UPDATE(i) (void)0 //create infinite loop
#endif

char quotes[5][BUFFER_MAX] = {
        "Always code as if the guy who ends up maintaining your code will be a violent psychopath who knows where you live.",
        "There are two ways to write error-free programs; only the third one works.",
        "Code is like humor. When you have to explain it, it’s bad.",
        "First, solve the problem. Then, write the code.",
        "Any fool can write code that a computer can understand. Good programmers write code that humans can understand."
    };

void print_help(void);

/**
 * For starting the sensor node 3 command line arguments are needed. These should be given in the order below
 * and can then be used through the argv[] variable
 *
 * argv[2] = sleep time
 * argv[3] = server IP
 * argv[4] = server port
 */

int main(int argc, char *argv[]) {
    int server_port, index;
    char server_ip[] = "000.000.000.000";
    tcpsock_t *client;
    int i, bytes, sleep_time;

    if (argc != 4) {
        print_help();
        exit(EXIT_SUCCESS);
    } else {
        // to do: user input validation!
        sleep_time = atoi(argv[1]);
        strncpy(server_ip, argv[2], strlen(server_ip));
        server_port = atoi(argv[3]);
    }

    srand(time(NULL));

    // open TCP connection to the server; server is listening to SERVER_IP and PORT
    if (tcp_active_open(&client, server_port, server_ip) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    i = LOOPS;
    while (i) {
        index = rand() % 5;
        bytes = strlen(quotes[index]);
        printf("Sending: %s\n",quotes[index]);
        if (tcp_send(client, (void *) &(quotes[index]), &bytes) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        sleep(sleep_time);
        UPDATE(i);
    }
    if (tcp_close(&client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}

/**
 * Helper method to print a message on how to use this application
 */
void print_help(void) {
    printf("Use this program with 3 command line options: \n");
    printf("\t%-15s : node sleep time (in sec) between two messages\n", "\'sleep time\'");
    printf("\t%-15s : TCP server IP address\n", "\'server IP\'");
    printf("\t%-15s : TCP server port number\n", "\'server port\'");
}
