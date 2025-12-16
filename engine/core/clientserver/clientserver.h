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



int sendstuff();

#endif