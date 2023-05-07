#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "custom_structs.h"
#include "helpers.h"
#include "common.h"

#define MAX_LEN 50

void print_message(udp_message message) {
    /* Print topic */
    printf("Received UDP message with topic %s and data type %c.\n", message.topic, message.type);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("\nUsage: %s <ID_Client> <IP_Server> <Port_Server>\n", argv[0]);
        return 1;
    }

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    /* Clean buffers */
    sockaddr_in serv_addr;
    socklen_t serv_addr_len = sizeof(serv_addr);
    memset(&serv_addr, 0, serv_addr_len);

    /* Create socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    /* Configure server address */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    int rc = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(rc == 0, "inet_aton");

    /* Connect to server */
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, serv_addr_len);
    DIE(rc < 0, "connect");

    /* Send client ID */
    char* id = argv[1];
    subscribe_message message;
    memcpy(message.client_id, id, strlen(id) + 1);
    message.action = 'i';
    message.topic[0] = '\0';
    message.SF = 0;

    rc = send_all(sockfd, &message, sizeof(subscribe_message));

    /* Create poll */
    struct pollfd fds[2];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    /* Infinite loop for commands */
    while (1) {
        /* Poll */
        rc = poll(fds, 2, -1);
        DIE(rc < 0, "poll");

        if (rc == 0) {
            continue;
        }

        /* Check if server sent a message */
        if (fds[0].revents & POLLIN) {
            /* Receive message */
            void* buffer = malloc(sizeof(udp_message));
            rc = recv(sockfd, &buffer, sizeof(udp_message), 0);
            DIE(rc < 0, "recv");

            /* Check for exit signals */
            if (*(int*)buffer == -1) {
                /* Close socket */
                close(sockfd);

                /* Free buffer */
                free(buffer);
                break;
            }

            /* Convert to UDP message */
            udp_message message = *(udp_message*)buffer;

            /* Print message */
            print_message(message);

            /* Free buffer */
            free(buffer);
        }

        /* Check if user sent a command */
        if (fds[1].revents & POLLIN) {
            /* Read command */
            char buffer[MAX_LEN];
            scanf("%100s", buffer);

            /* Create command */
            subscribe_message command;


            if (strcmp(buffer, "exit") == 0) {
                /* Exit */
                /* Announce server */
                command.action = 'e';
                command.topic[0] = '\0';
                command.SF = 0;

                rc = send_all(sockfd, &command, sizeof(subscribe_message));

                /* Close socket */
                close(sockfd);
                break;
            } else if (strcmp(buffer, "subscribe") == 0) {
                /* Subscribe */
                /* Read topic */
                memset(buffer, 0, MAX_LEN);
                scanf("%50s", buffer);
                memcpy(command.topic, buffer, strlen(buffer) + 1);

                /* Read SF */
                memset(buffer, 0, MAX_LEN);
                scanf("%50s", buffer);

                if (strcmp(buffer, "0") == 0) {
                    command.SF = 0;
                } else if (strcmp(buffer, "1") == 0) {
                    command.SF = 1;
                } else {
                    /* Invalid SF */
                    fprintf(stderr, "Invalid SF.\n");
                    continue;
                }

                /* Announce server */
                command.action = 's';
                rc = send_all(sockfd, &command, sizeof(subscribe_message));

                printf("Subscribed to topic.\n");
            } else if (strcmp(buffer, "unsubscribe") == 0) {
                /* Unsubscribe */
                /* Read topic */
                memset(buffer, 0, MAX_LEN);
                scanf("%50s", buffer);
                strcpy(command.topic, buffer);

                /* Announce server */
                command.action = 'u';
                command.SF = 0;

                rc = send_all(sockfd, &command, sizeof(subscribe_message));
                printf("Unsubscribed from topic.\n");
            } else {
                /* Invalid command */
                fprintf(stderr, "Invalid command.\n");
            }
        }
    }

	return 0;
}
