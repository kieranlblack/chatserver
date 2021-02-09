#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/unistd.h>

#include "client.h"

struct segment_t send_segment;

int main(int argc, char **argv) {
    if (argc != 3) {
        puts("Correct usage: ./client <username> <password>");
        return EXIT_FAILURE;
    }

    strncpy(send_segment.header.username, argv[1], MAX_USERNAME_LEN);
    strncpy(send_segment.header.password, argv[2], MAX_PASSWORD_LEN);

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

    pthread_t handle_input_thread;
    pthread_create(&handle_input_thread, NULL, (void *) handle_input, &sockfd);
    pthread_t handle_recieve_thread;
    pthread_create(&handle_recieve_thread, NULL, (void *) handle_recieve, &sockfd);

    pthread_join(handle_input_thread, NULL);
    pthread_join(handle_recieve_thread, NULL);

    perror("end");
    return EXIT_FAILURE;
}

void *handle_recieve(int *fd) {
    for (;;) {
        if (read(*fd, &segment, SEGMENT_LEN) < -1) {
            perror("read");
            return (void *) -1;
        }

        printf("%s", segment.body);
    }
}

void *handle_input(int *fd) {
    char input_buf[BODY_LEN] = { 0x0 };

    for (;;) {
        bzero(input_buf, BODY_LEN);
        printf("<%s> ", send_segment.header.username);
        if (!fgets(input_buf, BODY_LEN, stdin)) {
            perror("fgets");
            return (void *) -1;
        }

        strncpy(send_segment.body, input_buf, BODY_LEN);

        if (write(*fd, &send_segment, SEGMENT_LEN) < 0) {
            perror("write");
            return (void *) -1;
        }
    }
}
