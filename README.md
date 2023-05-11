Copyright 2023 - 2024 Omer Tarik Ilhan 324CA

# TCP Server and Client

## Description
This project represents and implementation of a TCP Server and Client.
The server provides a service for clients to connect to and send
subscribe or unsubscribe requests. The server will then send messages
to the clients that are subscribed to the topic of the message.

The messages are received by the server via an UDP connection. The
server will then send the messages to the clients via TCP.

The server will be able to handle multiple clients at the same time,
and will be able to send messages to all the clients that are
subscribed to a topic. The clients will be able to subscribe or
unsubscribe to a topic, and will be able to receive messages from the
server.

A client will be able to subscribe to a topic, having the option to
receive all the messages that were sent to that topic while the client
was disconnected, based on a SF (Store-Forward) strategy, as long as
the messages were sent after the client subscribed to the topic.

## Usage

Use the Makefile to compile the project. The Makefile has the following
commands:

- make build - compiles the project
- make clean - removes the compiled files
- make server - compiles the server
- make subscriber - compiles the client

To run the server, use the following command:

```bash
./server <PORT>
```

To run the client, use the following command:

```bash
./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>
```

## Implementation Details

### **The Server**

The server creates a socket and binds it to the specified port for the
TCP connection. It then creates a socket and binds it to the specified
port for the UDP connection. It then waits for clients to connect to
the server.

When a client connects to the server, the server creates a new socket
for the client and adds it to the list of sockets. It then waits for
events on the sockets. When an event occurs, the server handles it
accordingly.

When a client sends a message to the server, the server checks if the
message is a subscribe, unsubscribe or exit message. If it is a
subscribe or unsubscribe message, the server adds or removes the
client from the list of clients that are subscribed to the topic. If
it is an exit message, the server removes the client from the list of
sockets and closes the socket.

When a message is received from the UDP connection, the server
searches for the clients that are subscribed to the topic of the
message. It then sends the message to the clients that are subscribed
to the topic. If the client is not connected and the subscriber has
the SF option set, the server stores the message in a list of messages
that will be sent to the client when it connects to the server.

The main storage structures used by the server are:

- **topics** - a map that stores the topics and the clients that are
  subscribed to them

- **users** - a map that stores the clients' IDs and the sockets that
  are connected to them

- **messages** - a map that stores the clients' IDs and the messages
  that will be sent to them when they connect to the server

### **The Client**

The client creates a socket and connects to the server. It then waits
for events on the socket. When an event occurs, the client handles it
accordingly.

When the client receives a message from the server, it prints the
message to the standard output, based on the format specified in the
message data type.

When the client sends a message to the server, it checks if the
message is a subscribe, unsubscribe or exit message. If it is a
subscribe or unsubscribe message, the client sends the message to the
server. If it is an exit message, the client announces the server that
it will disconnect.

## Data Types

The messages sent between the server and the UDP clients are of the
following data type:

```
struct udp_message {
    char topic[50];
    uint8_t type;
    char payload[1500];
};
```
This data type is used to send messages from the UDP clients to the
server.

The messages sent between the server and the TCP clients are of the
following data type:

```
struct udp_message_with_addr {
    char topic[51];
    uint8_t type;
    char payload[1500];
    sockaddr_in client;
};
```

The above data type is used to send messages from the server to the
TCP clients, about the messages received from the UDP clients. The
message also contains the address of the client that sent the message
to the server, even though the clients don't use this information.
(The original plan was to use the address along with the topic, type,
and payload to send the message to the clients, but the checker
was not in sync with the project specifications)

```
struct subscribe_message {
    char action;
    char topic[51];
    char client_id[10];
    uint8_t SF;
};
```

Lastly, the above data type is used to send messages from the TCP
clients to the server. The message contains the action (subscribe,
unsubscribe, exit), the topic, the client ID and the SF option.

Based the type of the data type, the client will know how to handle
the message (**udp_message_with_addr**). These formats are the following:
- **INT** - 1 byte for the sign, 4 bytes for the module of the number
    (integer)
- **SHORT_REAL** - 2 bytes for the whole part, 2 bytes for the decimal
    part (2 decimal positive number)
- **FLOAT** - 1 byte for the sign, 4 bytes for the module of the
    number, 2 bytes for the number of decimals (float)
- **STRING** - 1500 bytes for the string

## Commands

### **Server**

```
exit
```

### **Client**

```
subscribe <topic> <SF>
unsubscribe <topic>
exit
```

When either the server or the client receive invalid commands, they
will print an error message to the standard error output.

## Comments and Observations

- This project was my first time using the socket API to implement
    a server and a client with multiple connections, that had an
    actual use case. I had to learn how to use the API and how to
    interface an UDP connection with a TCP connection.

- I learned how to multiplex the sockets and how to handle the
    messages received from the UDP connection.

- It was also my first time using C and C++ together, in the
    same project. I had to learn how to interface the two languages
    and how to use the C++ standard library properly.

- The most challenging part of the project was to use the socket API,
    with it's unintuitive syntax, functions and data types.

## References

[I/O Multiplexing](https://pcom.pages.upb.ro/labs/lab7/multiplexing.html)

[TCP Client](https://pcom.pages.upb.ro/labs/lab7/client.html)

[TCP Server](https://pcom.pages.upb.ro/labs/lab7/server.html)

[UDP Client / Server](https://pcom.pages.upb.ro/labs/lab5/client_serv.html)

[STL and C++](https://www.geeksforgeeks.org)

All the information accumulated within the Communication Protocols
course and laboratory.