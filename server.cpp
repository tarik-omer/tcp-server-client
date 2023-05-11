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

void run_server(int listenfd_udp, int listenfd_tcp)
{
	int num_clients = 0;
	int rc;

	/* Unordered map for current topics */
	std::map<std::string, topic_t> topics;

	/* Unordered map for 'online' users */
	std::map<std::string, int> users;

	/* Message storage */
	std::map<std::string, std::vector<udp_message_with_addr>> messages;

	/* Set socket to listen for incoming connections */
	rc = listen(listenfd_tcp, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");

	/* Initialize pollfd array */
	struct pollfd poll_fds[MAX_CONNECTIONS];
	memset(poll_fds, 0, sizeof(poll_fds));

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
						/* Close all sockets */
						for (int j = 3; j < num_clients; j++) {
							close(poll_fds[j].fd);
						}
						std::cerr << "Server closed." << std::endl;
						return;
					} else {
						/* Invalid command */
						std::cerr << "Invalid command." << std::endl;
					}
				}
				
				if (poll_fds[i].fd == listenfd_tcp) {
					/* New TCP connection */
					struct sockaddr_in client_addr;
					socklen_t client_addr_len = sizeof(client_addr);
					int newsockfd = accept(listenfd_tcp, (struct sockaddr *)&client_addr, &client_addr_len);
					DIE(newsockfd < 0, "accept");

					/* Receive client ID */
					subscribe_message message;
					memset(&message, 0, sizeof(subscribe_message));
					rc = recv_all(newsockfd, &message, sizeof(subscribe_message));
					DIE(rc < 0, "recv_all");

					/* Convert client ID to string */
					std::string client_id(message.client_id);

					/* Check if client ID exists */
					bool error = false;
					if (users.find(client_id) != users.end()) {
						/* Client ID exists */
						error = true;

						/* Close socket */
						close(newsockfd);

						/* Print already connected */
						std::cout << "Client " << client_id << " already connected." << std::endl;
					} else {
						/* Client ID does not exist - add new socket to poll */
						poll_fds[num_clients].fd = newsockfd;
						poll_fds[num_clients].events = POLLIN;
						num_clients++;

						/* Add client ID to map */
						users[client_id] = newsockfd;
					}

					if (error) {
						break;
					}
					/* Print new client */
					std::cout << "New client " << client_id
					<< " connected from " << inet_ntoa(client_addr.sin_addr)
					<< ":" << ntohs(client_addr.sin_port) << "." << std::endl;
				
					/* Send stored messages */
					if (messages.find(client_id) != messages.end()) {
						/* Client has stored messages */
						std::vector<udp_message_with_addr> client_messages = messages[client_id];

						for (size_t j = 0; j < client_messages.size(); j++) {
							/* Send message */
							rc = send(newsockfd, &client_messages[j], sizeof(udp_message_with_addr), 0);
							DIE(rc < 0, "send");
						}

						/* Delete stored messages */
						messages.erase(client_id);
					}
				}

				if (poll_fds[i].fd == listenfd_udp) {
					/* New UDP message */
					udp_message message;
					struct sockaddr_in client_addr;
					socklen_t len = sizeof(client_addr);

					/* Receive message */
					rc = recvfrom(listenfd_udp, &message, sizeof(udp_message), 0, (struct sockaddr *)&client_addr, &len);
					DIE(rc < 0, "recvfrom");

					/* Convert topic to string */
					std::string topic_name(message.topic);

					topic_t topic;

					/* Check if topic exists */
					if (topics.find(topic_name) != topics.end()) {
						/* Topic exists */
						topic = topics[topic_name];
					} else {
						/* Topic does not exist */
						break;
					}

					/* Pack message */
					udp_message_with_addr message_with_addr;
					memset(&message_with_addr, 0, sizeof(udp_message_with_addr));
					memcpy(&message_with_addr.data, &message.data, sizeof(message.data));
					memcpy(&message_with_addr.topic, &message.topic, sizeof(message.topic));
					memcpy(&message_with_addr.type, &message.type, sizeof(message.type));

					/* Send messages to all subscribed users */
					for (auto it = topic.users.begin(); it != topic.users.end(); it++) {
						/* Get subscriber ID */
						std::string subscriber_id = it->first;

						/* Check if subscriber is connected */
						if (users.find(subscriber_id) == users.end()) {
							/* Subscriber is not connected, but is subscribed to topic
							=> check if subscriber has SF flag set */
							if (it->second == 1) {
								/* Subscriber has SF flag set */
								/* Add message to queue */
								messages[subscriber_id].push_back(message_with_addr);
							}
							continue;
						}

						/* Get subscriber socket */
						int sub_fd = users[subscriber_id];
						
						/* Send message to subscriber */
						rc = send_all(sub_fd, &message_with_addr, sizeof(udp_message_with_addr));
						DIE(rc < 0, "send_to_subscriber");
					}
				}

				if (poll_fds[i].fd != listenfd_tcp
					&& poll_fds[i].fd != listenfd_udp
					&& poll_fds[i].fd != STDIN_FILENO
					&& poll_fds[i].fd != -1
					&& poll_fds[i].revents & POLLIN) {
					/* TCP Client is sending a 'request' (subscribe / unsubscribe / exit) - receive packet */
					pollfd current_client = poll_fds[i];
					subscribe_message message;
					memset(&message, 0, sizeof(subscribe_message));
					rc = recv_all(current_client.fd, &message, sizeof(subscribe_message));
					DIE(rc < 0, "recv_all");

					/* Check whether the client subscribes / unsubscribes */
					if (message.action == 's') {
						/* Client subscribes - check if topic exists */
						std::string topic_name(message.topic);
						if (topics.find(topic_name) == topics.end()) {
							/* Topic does not exist - create new topic */
							topic_t new_topic; 

							std::cerr << "Topic " << topic_name << " does not exist. Creating new topic." << std::endl;

							/* Initialize topic */
							new_topic.name = topic_name;

							/* Add topic to map */
							topics[topic_name] = new_topic;
						} else {
							/* Topic exists - check if subscriber is already subscribed */
							if (topics[topic_name].users.find(message.client_id) != topics[topic_name].users.end()) {
								/* Subscriber is already subscribed - will update SF */
								std::cerr << "Subscriber " << message.client_id << " is already subscribed to topic " << topic_name << std::endl;
							}
						}
						std::cerr << "Client " << message.client_id << " subscribes to " << topic_name << " with SF: ";
						fprintf(stderr, "%hhd\n", message.SF);
						/* Add subscriber to topic / update SF */
						topics[topic_name].users[message.client_id] = message.SF;
					} else if (message.action == 'u') {
						/* Client unsubscribes - check if topic exists */
						std::string topic_name = message.topic;
						if (topics.find(topic_name) != topics.end()) {
							/* Topic exists - check to see if user is subscriber to topic */
							if (topics[topic_name].users.find(message.client_id) != topics[topic_name].users.end()) {
								/* Remove subscriber from topic */
								topics[topic_name].users.erase(message.client_id);
							} else {
								/* User is not subscriber */
								std::cerr << "Client " << message.client_id << " is not subscribed to topic " << topic_name << std::endl;
								break;
							}

						} else {
							/* Topic does not exist */
							std::cerr << "Topic " << topic_name << " does not exist." << std::endl;
							break;
						}
					} else if (message.action == 'e') {
						/* Client exits - remove user */
						for (auto it = users.begin(); it != users.end(); it++) {
							if (it->second == current_client.fd) {
								/* Found client - remove from map and from stored messages */
								std::string client_id = it->first;
								std::cout << "Client " << client_id << " disconnected." << std::endl;
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

						/* Close socket */
						close(current_client.fd);
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		std::cout << "Usage: ./server <PORT>" << std::endl;
		return 1;
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	/* Parse port */
	uint16_t port;
	port = atoi(argv[1]);
	DIE(port == 0, "atoi");

	/* Create TCP socket */
	int listenfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listenfd_tcp < 0, "socket");

	/* Create UDP socket */
	int listenfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(listenfd_udp < 0, "socket");

	/* Server address */
	struct sockaddr_in server_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	/* Configure server address */
	memset(&server_addr, 0, socket_len);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	/* Enable the sockets to reuse the address */
	int enable = 1;

	if (setsockopt(listenfd_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");
	if (setsockopt(listenfd_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	/* Disable Nagle algorithm */
	int rc = setsockopt(listenfd_tcp, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt");

	/* Bind the TCP socket */
	rc = bind(listenfd_tcp, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	DIE(rc < 0, "bind");

	/* Bind the UDP socket */
	rc = bind(listenfd_udp, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	DIE(rc < 0, "bind");

	/* Run the server */
	run_server(listenfd_udp, listenfd_tcp);

	/* Close TCP socket */
	close(listenfd_tcp);

	/* Close UDP socket */
	close(listenfd_udp);

	return 0;
}