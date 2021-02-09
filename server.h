#ifndef __SERVER_H__
#define __SERVER_H__

#include "defs.h"

#define PASSWORD "password"
#define MAX_CLIENTS 10
#define MAX_EVENTS MAX_CLIENTS + 1

struct client_t {
    int fd;
    struct client_t *next;
};

int accept_client(int listenfd, int epollfd);
int remove_client(int fd, int epollfd);
int read_from_client(int fd);
int broadcast_to_clients(char *str, int fd);
int send_to_client(char *str, int fd);

#endif
