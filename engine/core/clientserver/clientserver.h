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
#include <signal.h>
#include <errno.h>
#include <string.h>


typedef enum {
    EXPECT_DATA = 31, 
    EXPECT_ARRAY = 32, 
    EXPECT_CLAIM_WORK = 33,
    GET_ARRAY = 34,
    GET_DATA = 35
} mssg_type;
#define MAGIC_NUMBER 0xA1B2C3D4
typedef enum {
    DATA_NOT_AVAILABLE = 41,
    ARRAY_NOT_AVAILABLE = 42,
    PROMISE_EMPTY = 43
} response_type;

typedef struct{
    PromiseStore *store;
    int client_fd;

}server_arg;


ssize_t write_all(int fd, const void *buf, size_t count);
ssize_t read_all(int fd, void *buf, size_t count);
void* handle_client_thread(void* arg);
int sendstuff();
int claim_work_client(int sock, char *key, int max_retries);
bool check_magic_number(uint8_t *buffer);
bool check_connection_alive(int sock);
#endif