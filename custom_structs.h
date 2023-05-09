#ifndef CUSTOM_STRUCTS_H
#define CUSTOM_STRUCTS_H

#include <netinet/in.h>
#include <map>
#include "common.h"
#include <string>

typedef struct
{
    uint8_t sign;
    uint32_t value;
} __attribute__((packed)) INT_T;

typedef struct
{
    uint16_t value;
} __attribute__((packed)) SHORT_REAL_T;

typedef struct
{
    uint8_t sign;
    uint32_t value;
    uint8_t power;
} __attribute__((packed)) FLOAT_T;

typedef struct
{
    char topic[50];
    uint8_t type;
    char data[1500];
} __attribute__((packed)) udp_message;

typedef struct
{
    char topic[51];
    uint8_t type;
    char data[1500];
    sockaddr_in client_addr;
} __attribute__((packed)) udp_message_with_addr;

typedef struct
{
    char action;
    char topic[51];
    char client_id[10];
    uint8_t SF;
} __attribute__((packed)) subscribe_message;

typedef struct
{
    std::string name;
    std::map<std::string, uint8_t> users;
} topic_t;

#endif