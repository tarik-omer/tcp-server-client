#include "common.h"
#include "helpers.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include "custom_structs.h"

/* Receive a full message from socket sockfd, storing it in buffer. */
int recv_all(int sockfd, void *buffer, size_t len) {
	size_t bytes_received = 0;
	size_t bytes_remaining = len;
	char *buff = (char*)buffer;

	while (bytes_remaining) {
		int rc = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
		DIE(rc == -1, "recv() failed!\n");
		if (rc == 0)
			return bytes_received;

		bytes_received += rc;
		bytes_remaining -= rc;
	}

  return bytes_received;
}

/* Send a full message to socket sockfd, storing it in buffer. */
int send_all(int sockfd, void *buffer, size_t len) {
  	size_t bytes_sent = 0;
  	size_t bytes_remaining = len;
  	char *buff = (char*)buffer;

  	while (bytes_remaining) {
		int rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
	  	DIE(rc == -1, "send() failed!\n");
	  	if (bytes_sent == 0)
			return bytes_sent;

	  	bytes_sent += rc;
	  	bytes_remaining -= rc;
	}
  
  	return bytes_sent;
}

/* Convert INT to string */
char* int_to_string(INT_T int_t) {
	char* str = (char*)malloc(10);
	DIE(str == NULL, "malloc");

	int sign = 0;
	if (int_t.sign == 0) {
		sign = 1;
	} else {
		sign = -1;
	}

	int number = 0;
	number = int_t.value;

	sprintf(str, "%d", sign * number);

	return str;
}

/* Convert SHORT_REAL to string */
char* short_real_to_string(SHORT_REAL_T short_real_t) {
	char* str = (char*)malloc(10);
	DIE(str == NULL, "malloc");

	int number = 0;
	number = short_real_t.value;

	sprintf(str, "%d.%d", number / 100, number % 100);

	return str;
}

/* Convert FLOAT to string */
char* float_to_string(FLOAT_T float_t) {
	char* str = (char*)malloc(10);
	DIE(str == NULL, "malloc");

	int sign = 0;
	if (float_t.sign == 0) {
		sign = 1;
	} else {
		sign = -1;
	}

	int number = 0;
	number = float_t.value;

	double multiplier = 0;
	multiplier = pow(10, -float_t.power);

	sprintf(str, "%f", sign * number * multiplier);

	return str;
}

/* Convert STRING to string */
char* string_to_string(STRING_T string_t) {
	char* str = (char*)malloc(1501);
	DIE(str == NULL, "malloc");

	strcpy(str, string_t.value);

	return str;
}

/* Comparator for topics - to be usable in maps */
bool operator<(topic_t& t1, topic_t& t2) {
    return strcmp(t1.name, t2.name) < 0;
}