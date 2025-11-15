#ifndef SERVER_CLIENT_HEADERS
#define SERVER_CLIENT_HEADERS

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>
#define SOCKET_PATH "/tmp/aurora.sock"
#include "../../helpers/helpers.h"













typedef enum {EXPECT_DATA , EXPECT_ARRAY} mssg_type;
#define MAGIC_NUMBER 0xA1B2C3D4
typedef enum {
    RESPONSE_ACK,
    RESPONSE_NACK,
} response_type;




ssize_t write_all(int fd, const void *buf, size_t count);
ssize_t read_all(int fd, void *buf, size_t count);
void Server_init_multithread();

#endif