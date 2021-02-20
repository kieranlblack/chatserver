#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <time.h>

#include "client.h"

volatile sig_atomic_t sigint_received = false;

struct segment_t send_segment;
pthread_mutex_t sent_lock;
pthread_t handle_input_thread;
pthread_t handle_recieve_thread;

bool sent = false;
int sockfd;

int main(int argc, char **argv) {
    if (argc != 3) {
        puts("Correct usage: ./client <username> <password>");
        return EXIT_FAILURE;
    }

    strncpy(send_segment.header.username, argv[1], MAX_USERNAME_LEN);
    strncpy(send_segment.body, argv[2], MAX_PASSWORD_LEN);

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

    struct sigaction act = {
        .sa_handler = handle_sigint,
        .sa_flags = SA_RESTART
    };
    sigaction(SIGINT, &act, NULL);

    if (write(sockfd, &send_segment, SEGMENT_LEN) < -1) {
        perror("write");
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&sent_lock, NULL)) {
        perror("pthread_mutex_init");
    }

    pthread_create(&handle_input_thread, NULL, (void *) handle_input, NULL);
    pthread_create(&handle_recieve_thread, NULL, (void *) handle_recieve, NULL);

    pthread_join(handle_recieve_thread, NULL);

    pthread_mutex_destroy(&sent_lock);
    close(sockfd);
    return EXIT_SUCCESS;
}

void handle_sigint(int signum) {
    sigint_received = true;
    pthread_kill(handle_input_thread, SIGINT);
    pthread_kill(handle_recieve_thread, SIGINT);
}

void *handle_recieve() {
    printf("<%s> ", send_segment.header.username);
    fflush(stdout);

    while (!sigint_received) {
        bzero(&segment, SEGMENT_LEN);

        ssize_t n;
        if ((n = read(sockfd, &segment, SEGMENT_LEN)) < -1) {
            perror("read");
            return (void *) -1;
        }

        if (!n) {
            close(sockfd);
            return (void *) 1;
        }

        pthread_mutex_lock(&sent_lock);
        if (strncmp(segment.header.username, send_segment.header.username, MAX_USERNAME_LEN) && !sent) {
            printf("\n");
        }
        sent = 0;
        pthread_mutex_unlock(&sent_lock);

        printf("\033[F\033[J%.*s", (int) strnlen(segment.body, BODY_LEN) - 1, segment.body);
        printf("<%s> ", send_segment.header.username);
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

void *handle_input() {
    char input_buf[BODY_LEN] = { 0x0 };

    while (!sigint_received) {
        bzero(input_buf, BODY_LEN);

        if (!fgets(input_buf, BODY_LEN, stdin)) {
            perror("fgets");
            return (void *) -1;
        }

        if (transform_input(input_buf)) {
            close(sockfd);
            pthread_mutex_destroy(&sent_lock);
            exit(0);
        }

        bcopy(input_buf, &send_segment.body, BODY_LEN);

        pthread_mutex_lock(&sent_lock);
        sent = true;
        pthread_mutex_unlock(&sent_lock);

        if (write(sockfd, &send_segment, SEGMENT_LEN) < 0) {
            perror("write");
            return (void *) -1;
        }
    }
}
