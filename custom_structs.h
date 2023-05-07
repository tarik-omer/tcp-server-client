#ifndef CUSTOM_STRUCTS_H
#define CUSTOM_STRUCTS_H

#include <netinet/in.h>
#include <vector>

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
    char value[1500];
} __attribute__((packed)) STRING_T;

typedef struct
{
    char topic[50];
    uint8_t type;
    char data[1500];
} __attribute__((packed)) udp_message;

typedef struct
{
    char action;
    char client_id[10];
    uint8_t SF;
    char topic[50];
} __attribute__((packed)) subscribe_message;

typedef struct
{
    char name[50];
    bool is_sf;
    std::vector<std::tuple<char*, int, uint8_t>> users;
} topic_t;

#endif