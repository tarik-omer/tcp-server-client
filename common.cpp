#include "common.h"
#include "helpers.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <typeinfo>

#include "custom_structs.h"

#define MAX_NUM_LEN 20
#define MAX_STRING_LEN 1500

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

/* Convert data to INT_T */
char* int_to_str(char* data) {
	INT_T* int_data = (INT_T*)data;
	char* str = (char*)malloc(MAX_NUM_LEN * sizeof(char));

	int num = ntohl(int_data->value);

	int sign = int_data->sign == 1 ? -1 : 1;

	sprintf(str, "%d", sign * num);

	return str;
}

/* Convert data to SHORT_REAL_T */
char* short_real_to_str(char* data) {
	SHORT_REAL_T* short_real_data = (SHORT_REAL_T*)data;
	char* str = (char*)malloc(MAX_NUM_LEN * sizeof(char));

	int num = ntohs(short_real_data->value);

	if (num % 100 == 0) {
		sprintf(str, "%d", num / 100);
		return str;
	}

	if (num % 100 < 10) {
		sprintf(str, "%d.0%d", num / 100, num % 100);
		return str;
	}

	sprintf(str, "%d.%d", num / 100, num % 100);
	return str;
}

/* Convert data to FLOAT_T */
char* float_to_str(char* data) {
	FLOAT_T* float_data = (FLOAT_T*)data;
	char* str = (char*)malloc(MAX_NUM_LEN * sizeof(char));

	/* Get the number and the power */
	float num = ntohl(float_data->value);
	uint8_t power = float_data->power;

	/* Obtain the actual float value */
	while (power) {
		num /= 10.0f;
		power--;
	}
	
	/* Add the sign */
	int sign = float_data->sign == 1 ? -1.0f : 1.0f;

	sprintf(str, "%f", sign * num);
	return str;
}

/* Convert data to STRING_T */
char* string_to_str(char* data) {
	data[MAX_STRING_LEN - 1] = '\0';
	return data;
}
