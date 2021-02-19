#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <time.h>

#include "client.h"

struct segment_t send_segment;

int main(int argc, char **argv) {
    if (argc != 3) {
        puts("Correct usage: ./client <username> <password>");
        return EXIT_FAILURE;
    }

    strncpy(send_segment.header.username, argv[1], MAX_USERNAME_LEN);
    strncpy(send_segment.body, argv[2], MAX_PASSWORD_LEN);

    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in serveraddr = {
        .sin_family = AF_INET,
        .sin_port = htons(5001),
        .sin_zero = 0x0
    };
    if (inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr) != 1) {
        perror("inet_pton");
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        perror("connect");
        return EXIT_FAILURE;
    }

    if (write(sockfd, &send_segment, SEGMENT_LEN) < -1) {
        perror("write");
        return EXIT_FAILURE;
    }

    pthread_t handle_input_thread;
    pthread_create(&handle_input_thread, NULL, (void *) handle_input, &sockfd);
    pthread_t handle_recieve_thread;
    pthread_create(&handle_recieve_thread, NULL, (void *) handle_recieve, &sockfd);

    pthread_join(handle_recieve_thread, NULL);

    return EXIT_SUCCESS;
}

void *handle_recieve(int *fd) {
    printf("<%s> ", send_segment.header.username);
    fflush(stdout);

    for (;;) {
        bzero(&segment, SEGMENT_LEN);

        ssize_t n;
        if ((n = read(*fd, &segment, SEGMENT_LEN)) < -1) {
            perror("read");
            return (void *) -1;
        }

        if (!n) {
            close(*fd);
            return (void *) 1;
        }

        printf("\033[F\033[J%.*s", (int) strnlen(segment.body, BODY_LEN) - 1, segment.body);
        printf("\n<%s> ", send_segment.header.username);
        fflush(stdout);
    }
}

int transform_input(char *s) {
    if (!(*s) || *s != ':') {
        return 0;
    }

    uint hash = 0x1;
    for (char *ch = s; *ch; ch++) {
        hash += hash * 31 + *ch;
    }

    time_t current_time;
    struct tm *broken_down_time;
    int hour_offset = 0;
    switch (hash) {
    case HAPPY:
        strcpy(s, "[feeling happy]\n");
        break;
    case SAD:
        strcpy(s, "[feeling sad]\n");
        break;
    case TIME_PLUS_ONE_HOUR:
        hour_offset = 1;
    case TIME:
        time(&current_time);
        broken_down_time = localtime(&current_time);
        broken_down_time->tm_hour += hour_offset;
        mktime(broken_down_time);
        strcpy(s, asctime(broken_down_time));
        break;
    case EXIT:
        return -1;
        break;
    }

    return 0;
}

void *handle_input(int *fd) {
    char input_buf[BODY_LEN] = { 0x0 };

    for (;;) {
        bzero(input_buf, BODY_LEN);

        if (!fgets(input_buf, BODY_LEN, stdin)) {
            perror("fgets");
            return (void *) -1;
        }

        if (transform_input(input_buf)) {
            close(*fd);
            exit(0);
        }

        bcopy(input_buf, &send_segment.body, BODY_LEN);

        if (write(*fd, &send_segment, SEGMENT_LEN) < 0) {
            perror("write");
            return (void *) -1;
        }
    }
}
