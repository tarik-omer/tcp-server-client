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

#define MAX_LEN 100
#define MAX_TOPIC_LEN 50

void print_message(udp_message_with_addr message) {
    /* Print under format: <TOPIC> - <DATA_TYPE> - <DATA> */
    /* Topic - might be 50 characters long, so we need to add a null terminator */
    char topic[MAX_TOPIC_LEN + 1];
    memset(topic, 0, MAX_TOPIC_LEN + 1);
    strncpy(topic, message.topic, MAX_TOPIC_LEN);
    strcpy(topic + MAX_TOPIC_LEN, "\0");

    printf("%s - ", topic);

    /* Data type */
    char* data = NULL;
    switch (message.type) {
        case 0:
            printf("INT - ");
            data = int_to_str(message.data);
            break;
        case 1:
            printf("SHORT_REAL - ");
            data = short_real_to_str(message.data);
            break;
        case 2:
            printf("FLOAT - ");
            data = float_to_str(message.data);
            break;
        default:
            printf("STRING - ");
            data = string_to_str(message.data);
            break;
    }
    /* Print data */
    printf("%s\n", data);
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

    /* Create socket for TCP connection */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

	/* Disable Nagle algorithm */
    int enable = 1;
	int rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt");


    /* Configure server address */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    rc = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(rc == 0, "inet_aton");

    /* Connect to server */
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, serv_addr_len);
    DIE(rc < 0, "connect");

    /* Send client ID */
    char* id = argv[1];
    subscribe_message message;
    memset(&message, 0, sizeof(subscribe_message));
    memcpy(message.client_id, id, strlen(id));
    rc = send_all(sockfd, &message, sizeof(subscribe_message));

    /* Send dummy */
    memset(&message, 0, sizeof(subscribe_message));
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
            break;
            /* Server closed connection */
            fprintf(stderr, "Server closed connection.\n");

        }

        /* Check if server sent a message */
        if (fds[0].revents & POLLIN) {
            /* Receive message */
            udp_message_with_addr* buffer = (udp_message_with_addr*)malloc(sizeof(udp_message_with_addr));
            rc = recv_all(sockfd, buffer, sizeof(udp_message_with_addr));
            DIE(rc < 0, "recv");

            /* Check if server closed connection */
            if (rc == 0) {
                fprintf(stderr, "Server closed connection.\n");
                free(buffer);
                break;
            }

            /* Print message */
            print_message(*buffer);

            /* Free the message buffer */
            free(buffer);
        }

        /* Check if user sent a command */
        if (fds[1].revents & POLLIN) {
            /* Read command */
            char buffer[MAX_LEN];
            memset(buffer, 0, MAX_LEN);
            fgets(buffer, MAX_LEN - 1, stdin);
            buffer[strlen(buffer) - 1] = '\0';

            /* Check if command is valid */
            if (strlen(buffer) == 0) {
                continue;
            }

            /* Parse command */
            char* word = strtok(buffer, " ");

            /* Create command */
            subscribe_message command;
            memset(&command, 0, sizeof(subscribe_message));

            /* Set client ID */
            memcpy(command.client_id, id, strlen(id) + 1);

            if (strcmp(word, "exit") == 0) {
                /* Exit */
                command.action = 'e';
                rc = send_all(sockfd, &command, sizeof(subscribe_message));
                break;
            } else if (word == NULL) {
                /* Invalid command */
                fprintf(stderr, "Invalid command.\n");
            } else if (strcmp(buffer, "subscribe") == 0) {
                /* Subscribe */
                /* Read topic */
                word = strtok(NULL, " ");
                memcpy(command.topic, word, strlen(word) + 1);

                /* Read SF */
                word = strtok(NULL, " ");
                /* Check if SF is given */
                if (word == NULL) {
                    /* No SF given */
                    command.SF = 0;
                } else if (strcmp(word, "0") == 0) {
                    command.SF = 0;
                } else if (strcmp(word, "1") == 0) {
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
            } else if (strcmp(word, "unsubscribe") == 0) {
                /* Unsubscribe */
                /* Read topic */
                word = strtok(NULL, " ");
                strcpy(command.topic, word);

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
