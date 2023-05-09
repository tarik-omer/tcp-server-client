#ifndef __COMMON_H__
#define __COMMON_H__

#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include <sys/timerfd.h>

#include "custom_structs.h"

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);
char* int_to_str(char* data);
char* short_real_to_str(char* data);
char* float_to_str(char* data);
char* string_to_str(char* data);

/* Comparator for char* - to be usable in maps */
struct cmp_str {
	inline bool operator()(char const *a, char const *b) const {
		return strcmp(a, b) < 0;
	}
};

#endif
