#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>

#include "server.h"

struct epoll_event events[MAX_EVENTS];
int epollfd;

struct client_t *client = NULL;
int num_clients = 0;

int main(void) {
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int socketopt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &socketopt, sizeof(int)) < 0) {
        perror("setsocketopt");
        return EXIT_FAILURE;
    }

    struct sockaddr_in serveraddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(5001),
        .sin_zero = 0x0
    };
    if (bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    if (listen(listenfd, 2) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    if ((epollfd = epoll_create1(0)) < 0) {
        perror("epoll_create1");
        return EXIT_FAILURE;
    }

    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.fd = listenfd
    };
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev)) {
        perror("epoll_ctl: listenfd");
        return EXIT_FAILURE;
    }

    int nfds;
    for (;;) {
        if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) < 0) {
            perror("epoll_wait");
            return EXIT_FAILURE;
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == listenfd) {
                puts("new client connecting");
                if (accept_client(listenfd) < 0) {
                    return EXIT_FAILURE;
                }
                continue;
            }

            if (events[i].events & EPOLLRDHUP) {
                if (remove_client(fd) < 0) {
                    return EXIT_FAILURE;
                }
                continue;
            }

            printf("num clients: %d\n", num_clients);

            if (read_from_client(fd) < 0) {
                return EXIT_FAILURE;
            }
        }
    }

    perror("end");
    return EXIT_FAILURE;
};

int remove_client(int fd) {
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
        perror("epoll_ctl: del");
        return -1;
    }

    if (num_clients == 1) {
        free(client);
        client = NULL;
    } else {
        struct client_t *curr = client;
        while (curr->next && curr->next->fd != fd) {
            curr = curr->next;
        }

        free(curr->next);
        curr->next = NULL;
    }

    close(fd);

    num_clients--;
    return 1;
}

int accept_client(int listenfd) {
    if (num_clients > MAX_CLIENTS) {
        return 1;
    }

    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int childfd;
    if ((childfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen)) < 0) {
        perror("accept");
        return -1;
    }

    if (fcntl(childfd, F_SETFL, fcntl(childfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        perror("fcntl");
        return -1;
    }

    struct epoll_event ev = {
        .events = EPOLLIN | EPOLLRDHUP,
        .data.fd = childfd
    };
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, childfd, &ev) < 0) {
        perror("epoll_ctl: childfd");
        return -1;
    }

    if (!client) {
        if (!(client = malloc(sizeof(struct client_t)))) {
            perror("malloc");
            return -1;
        }

        client->fd = childfd;
        client->is_authenticated = false;
        client->next = NULL;
    } else {
        struct client_t *curr = client;
        while (curr->next) {
            curr = curr->next;
        }

        if (!(curr->next = malloc(sizeof(struct client_t)))) {
            perror("malloc");
            return -1;
        }

        curr->next->fd = childfd;
        curr->next->is_authenticated = false;
        curr->next->next = NULL;
    }

    num_clients++;
    return 1;
}

int broadcast_to_authenticated_clients(char *str) {
    bzero(&segment, BODY_LEN);
    strncpy(segment.body, str, BODY_LEN);

    for (struct client_t *curr = client; curr; curr = curr->next) {
        if (!client->is_authenticated) {
            continue;
        }

        if (write(curr->fd, &segment, SEGMENT_LEN) < 0) {
            fprintf(stderr, "client disconnected unexpectedly\n");
            remove_client(curr->fd);
        }
    }

    return 1;
}

int send_to_client(char *str, int fd) {
    bzero(&segment, BODY_LEN);
    strncpy(segment.body, str, BODY_LEN);

    if (write(fd, &segment, SEGMENT_LEN) < 0) {
        perror("write");
        return -1;
    }

    return 1;
}

int perform_checks(int fd) {
    int ret_flag = 0;

    struct client_t *curr = client;
    while (curr && curr->fd != fd) {
        curr = curr->next;
    }

    if (!curr) {
        fprintf(stderr, "fd does not exist\n");
        return -1;
    }

    if (!curr->is_authenticated) {
        if (strcmp(PASSWORD, segment.body)) {
            send_to_client("<SERVER> incorrect password\n", fd);
            return remove_client(fd);
        }

        curr->is_authenticated = true;
        bzero(segment.body, BODY_LEN);
        ret_flag = 1;
    }

    for (int i = 0; i < strnlen(segment.header.username, MAX_USERNAME_LEN); i++) {
        if (!isalnum(segment.header.username[i])) {
            send_to_client("<SERVER> username can only contain alphanumeric characters\n", fd);
            return remove_client(fd);
        }
    }

    return ret_flag;
}

char msg_buf[BODY_LEN];
int read_from_client(int fd) {
    bzero(&segment, SEGMENT_LEN);

    ssize_t n;
    if ((n = read(fd, &segment, SEGMENT_LEN)) < 0) {
        if (errno == EWOULDBLOCK && errno == EAGAIN) {
            return 1;
        }

        perror("read: child");
        return -1;
    }

    int check_res;
    if ((check_res = perform_checks(fd))) {
        return check_res;
    }

    bzero(msg_buf, BODY_LEN);
    snprintf(msg_buf, BODY_LEN, "<%s> %.*s\n", segment.header.username, (int) strnlen(segment.body, BODY_LEN), segment.body);
    fputs(msg_buf, stdout);

    return broadcast_to_authenticated_clients(msg_buf);
}
