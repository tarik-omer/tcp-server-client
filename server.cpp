#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <cmath>

#include "common.h"
#include "helpers.h"
#include "custom_structs.h"

#define MAX_LEN 50

#define MAX_CONNECTIONS 1000

int process_subscription() {
	return 0;
}

int process_unsubscription() {
	return 0;
}

int send_to_subscriber(int sub_fd, size_t len, udp_message* message)
{
	void* buff = malloc(len);
	if (!buff)
	{
		perror("malloc");
		return -1;
	}

	/* Copy message to buffer */
	memcpy(buff, message, len);

	/* Send message to subscriber */
	int rc = send_all(sub_fd, buff, len);
	DIE(rc < 0, "send_all");

	return 0;
}

void run_server(int listenfd_udp, int listenfd_tcp)
{

	struct pollfd poll_fds[MAX_CONNECTIONS];
	int num_clients = 0;
	int rc;

	/* Unordered map for current topics */
	std::map<char*, topic_t> topics;

	/* Unordered map for 'online' users */
	std::map<char*, int> users;

	// Setam socket-ul listenfd pentru ascultare
	rc = listen(listenfd_tcp, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");

	/* Add STDIN listener */
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	num_clients++;

	/* Add TCP listerner */
	poll_fds[1].fd = listenfd_tcp;
	poll_fds[1].events = POLLIN;
	num_clients++;

	/* Add UDP listerner */
	poll_fds[2].fd = listenfd_udp;
	poll_fds[2].events = POLLIN;
	num_clients++;

	while (1) {
		/* Poll for events - wait for readiness notification */
		int poll_rc = poll(poll_fds, num_clients, -1);
		DIE(poll_rc < 0, "poll");

		/* Check for events */
		for (int i = 0; i < num_clients; i++) {
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == STDIN_FILENO) {
					/* STDIN command */
					char buffer[MAX_LEN];
					fgets(buffer, MAX_LEN - 1, stdin);
					buffer[strlen(buffer) - 1] = '\0';

					/* Check for exit command */
					if (strcmp(buffer, "exit") == 0) {
						/* Send closing signal to all clients */
						subscribe_message exit_message;
						exit_message.action = 'e';

						for (int j = 3; j < num_clients; j++) {
							rc = send_all(poll_fds[j].fd, &exit_message, sizeof(subscribe_message));
							DIE(rc < 0, "send_all");
						}

						/* Close all sockets */
						for (int j = 3; j < num_clients; j++) {
							close(poll_fds[j].fd);
						}

						printf("Exiting...\n");
						return;
					} else {
						printf("Invalid command.\n");
					}
				} else if (poll_fds[i].fd == listenfd_tcp) {
					/* New TCP connection */
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int newsockfd = accept(listenfd_tcp, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");

					/* Add new socket to poll */
					poll_fds[num_clients].fd = newsockfd;
					poll_fds[num_clients].events = POLLIN;
					num_clients++;

					/* Receive client ID */
					subscribe_message message;
					rc = recv_all(newsockfd, &message, sizeof(subscribe_message));

					char* client_id = message.client_id;

					bool error = false;

					/* Check if client ID exists */
					if (users.find(client_id) != users.end()) {
						/* Client ID exists */
						error = true;

						/* Send error message */
						subscribe_message error_message;
						error_message.action = 'e';

						rc = send_all(newsockfd, &error_message, sizeof(subscribe_message));
						DIE(rc < 0, "send_all");

						/* Close socket */
						close(newsockfd);

						/* Remove socket from poll */
						num_clients--;

						/* Print already connected */
						printf("Client %s already connected.\n", client_id);
					} else {
						/* Client ID does not exist */
						/* Add client ID to map */
						users[client_id] = newsockfd;
					}

					if (error) {
						continue;
					}

					/* Print new client */
					printf("New client %s connected from %s:%hu.\n", client_id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				} else if (poll_fds[i].fd == listenfd_udp) {
					/* New UDP message */
					udp_message message;
					struct sockaddr_in cli_addr;
					socklen_t len = sizeof(cli_addr);

					/* Receive message */
					rc = recvfrom(listenfd_udp, &message, sizeof(udp_message), MSG_WAITALL, (struct sockaddr *)&cli_addr, &len);
					DIE(rc < 0, "recvfrom");
					
					/* Check if topic exists */
					char* topic_name = message.topic;
					topic_t topic;

					if (topics.find(topic_name) != topics.end()) {
						/* Topic exists */
						topic = topics[topic_name];
					}

					/* Send messages to all subscribed users */
					for (size_t j = 0; j < topic.users.size(); j++) {
						/* Get subscriber socket */
						int sub_fd = std::get<1>(topic.users[j]);

						/* Check if subscriber is connected */
						if (sub_fd == -1) {
							continue;
						}

						/* Send message to subscriber */
						rc = send_to_subscriber(sub_fd, sizeof(udp_message), &message);
						DIE(rc < 0, "send_to_subscriber");

						// TODO IMPORTANT: Store messages in buffer and send them when subscriber is ready
					}


				} else if (poll_rc > 0) {
					/* TCP Client is sending a subscribe / unsubscribe command */
					/* Receive packet */
					pollfd current_client = poll_fds[i];
					subscribe_message message;
					rc = recv_all(current_client.fd, &message, sizeof(subscribe_message));
					DIE(rc < 0, "recv_all");

					/* Check whether the client subscribes / unsubscribes */
					if (message.action == 's') {
						/* Client subscribes - check if topic exists */
						char* topic_name = message.topic;
						if (topics.find(topic_name) == topics.end()) {
							/* Topic does not exist */
							/* Create new topic */
							topic_t new_topic; 

							/* Initialize topic */
							memcpy(new_topic.name, topic_name, 50);

							/* Add topic to map */
							topics[topic_name] = new_topic;

							/* Add subscriber to topic */
							new_topic.users.push_back(std::make_tuple(message.client_id, current_client.fd, message.SF));
						} else {
							/* Topic exists - add subscriber to topic */
							topics[topic_name].users.push_back(std::make_tuple(message.client_id, current_client.fd, message.SF));
						}
						/* Add SF */
						if (message.SF == 1) {
							/* Add SF */
							topics[topic_name].is_sf = true;
						}

						/* Send messages from buffer */
						// TODO

					} else if (message.action == 'u') {
						/* Client unsubscribes - check if topic exists */
						char* topic_name = message.topic;
						if (topics.find(topic_name) != topics.end()) {
							/* Topic exists */
							/* Find ID index */
							int client_idx = -1;
							for (size_t j = 0; j < topics[topic_name].users.size(); j++) {
								char* id = std::get<0>(topics[topic_name].users[j]);
								if (strcmp(id, message.client_id) == 0) {
									/* Found ID */
									client_idx = j;
									break;
								}
							}

							/* Remove client from topic */
							if (client_idx != -1) {
								topics[topic_name].users.erase(topics[topic_name].users.begin() + client_idx);
							} else {
								/* Client is not subscribed to topic */
								fprintf(stderr, "Client %s is not subscribed to topic %s\n", message.client_id, topic_name);
							}
						} else {
							/* Topic does not exist */
							fprintf(stderr, "Topic %s does not exist\n", topic_name);
						}
					} else if (message.action == 'e') {
						/* Client exits - remove user */
						for (auto it = users.begin(); it != users.end(); it++) {
							if (it->second == current_client.fd) {
								/* Found client */
								printf("Client %s disconnected.\n", it->first);
								users.erase(it);
								break;
							}
						}

						/* Remove client from poll */
						for (int j = 0; j < num_clients; j++) {
							if (poll_fds[j].fd == current_client.fd) {
								/* Found client */
								poll_fds[j] = poll_fds[num_clients - 1];
								num_clients--;
								break;
							}
						}

						/* Send exit signal */
						subscribe_message exit_message;
						exit_message.action = 'e';
						rc = send_all(current_client.fd, &exit_message, sizeof(subscribe_message));

						/* Print exit message */
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("\nUsage: %s <port>\n", argv[0]);
		return 1;
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	/* Parse port */
	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	/* Create TCP socket */
	int listenfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listenfd_tcp < 0, "socket");

	/* Create UDP socket */
	int listenfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(listenfd_udp < 0, "socket");

	/* Server address */
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	/* Enable the sockets to reuse the address */
	int enable = 1;

	if (setsockopt(listenfd_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");
	if (setsockopt(listenfd_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	/* Disable Nagle algorithm */
	rc = setsockopt(listenfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
	DIE(rc < 0, "setsockopt");

	/* Configure server address */
	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	/* Bind the TCP socket */
	rc = bind(listenfd_tcp, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bind");

	/* Bind the UDP socket */
	rc = bind(listenfd_udp, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bind");

	/* Run the server */
	run_server(listenfd_udp, listenfd_tcp);

	/* Close TCP socket */
	close(listenfd_tcp);

	/* Close UDP socket */
	close(listenfd_udp);

	return 0;
}