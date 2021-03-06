#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdbool.h>
#include "defs.h"

#define PASSWORD "cs3251secret"
#define MAX_CLIENTS 512
#define MAX_EVENTS MAX_CLIENTS + 1

struct client_t {
    int fd;
    bool is_authenticated;
    struct client_t *next;
};

int accept_client(int listenfd);
int remove_client(int fd);
int read_from_client(int fd);
int broadcast_to_authenticated_clients(char *str);
int send_to_client(char *str, int fd);

#endif
