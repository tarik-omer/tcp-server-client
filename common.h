#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/timerfd.h>
#include "custom_structs.h"

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);
char* int_to_string(INT_T int_t);
char* short_real_to_string(SHORT_REAL_T short_real_t);
char* float_to_string(FLOAT_T float_t);
char* string_to_string(STRING_T string_t);
bool operator<(topic_t& t1, topic_t& t2);

#endif
