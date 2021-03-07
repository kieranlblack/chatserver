#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
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

#include "chatclient.h"

volatile sig_atomic_t sigint_received = false;

struct segment_t send_segment;
pthread_mutex_t sent_lock;
pthread_t handle_input_thread;
pthread_t handle_recieve_thread;

bool sent = false;
int sockfd;

struct {
    char *username;
    char *password;
    char *host;
    char *port;
} args = {
    NULL, NULL, NULL, NULL
};

int main(int argc, char **argv) {
    if (argc != 9) {
        puts("Correct usage:");
        puts("\t./client \\");
        puts("\t\t --username <username> \\");
        puts("\t\t --password cs3251secret \\");
        puts("\t\t --host 127.0.0.1 \\");
        puts("\t\t --port 5001");
        return EXIT_FAILURE;
    }

    static struct option long_options[] = {
        {"username", required_argument, 0, 0 },
        {"password", required_argument, 0, 0 },
        {"host",     required_argument, 0, 0 },
        {"port",     required_argument, 0, 0 },
        {0,          0,                 0, 0 }
    };

    // parse command line arguments
    char **arg;
    char c;
    int option_index;
    for (;;) {
        if ((c = getopt_long_only(argc, argv, "", long_options, &option_index)) < 0) {
            break;
        }

        switch (c) {
        case 0:
            arg = (char **) ((char *) &args + option_index * sizeof(char *));
            if (!(*arg = malloc(MAX_USERNAME_LEN * sizeof(char)))) {
                perror("malloc");
                return EXIT_FAILURE;
            }

            strncpy(*arg, optarg, MAX_USERNAME_LEN);
            break;
        }
    }

    if (!args.username || !args.password || !args.host || !args.port) {
        printf("Missing 1 or more required arguments!\n");
        return EXIT_SUCCESS;
    }

    strncpy(send_segment.header.username, args.username, MAX_USERNAME_LEN);
    strncpy(send_segment.body, args.password, MAX_PASSWORD_LEN);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in serveraddr = {
        .sin_family = AF_INET,
        .sin_port = htons(strtol(args.port, NULL, 10)),
        .sin_zero = 0x0
    };
    if (inet_pton(AF_INET, args.host, &serveraddr.sin_addr) != 1) {
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

    // send password
    if (write(sockfd, &send_segment, SEGMENT_LEN) < 0) {
        perror("write");
        return EXIT_FAILURE;
    }

    printf("connected to %s on port %s\n", args.host, args.port);

    if (pthread_mutex_init(&sent_lock, NULL)) {
        perror("pthread_mutex_init");
    }

    pthread_create(&handle_input_thread, NULL, (void *) handle_input, NULL);
    pthread_create(&handle_recieve_thread, NULL, (void *) handle_recieve, NULL);

    pthread_join(handle_recieve_thread, NULL);

    wrap_up();
    return EXIT_FAILURE;
}

void handle_sigint(int signum) {
    sigint_received = true;
}

void *handle_recieve() {
    struct sigaction act = {
        .sa_handler = handle_sigint,
        .sa_flags = SA_RESTART
    };
    sigaction(SIGINT, &act, NULL);

    printf("<%s> ", send_segment.header.username);
    fflush(stdout);

    while (!sigint_received) {
        bzero(&segment, SEGMENT_LEN);

        ssize_t n;
        if ((n = read(sockfd, &segment, SEGMENT_LEN)) < 0) {
            if (errno == EINTR) {
                wrap_up();
            }

            perror("read");
            return (void *) -1;
        }

        if (!n) {
            wrap_up();
            return (void *) 1;
        }

        pthread_mutex_lock(&sent_lock);
        if (strncmp(segment.header.username, send_segment.header.username, MAX_USERNAME_LEN) && !sent) {
            printf("\n");
        }
        sent = 0;
        pthread_mutex_unlock(&sent_lock);

        printf("\033[F\033[J%.*s", (int) strnlen(segment.body, BODY_LEN), segment.body);
        printf("<%s> ", send_segment.header.username);
        fflush(stdout);
    }
}

void wrap_up(void) {
    close(sockfd);
    pthread_mutex_destroy(&sent_lock);
    printf("\n");
    exit(0);
}

uint hash_string(char *s) {
    uint hash = 0x1;
    for (char *ch = s; *ch; ch++) {
        hash += hash * 31 + *ch;
    }

    return hash;
}

int transform_input(char *s) {
    if (!(*s) || *s != ':') {
        return 0;
    }

    uint hash = hash_string(s);

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
    struct sigaction act = {
        .sa_handler = handle_sigint,
        .sa_flags = SA_RESTART
    };
    sigaction(SIGINT, &act, NULL);

    char input_buf[BODY_LEN] = {};
    uint16_t chars_input;

    char c;
    while (!sigint_received) {
        bzero(input_buf, BODY_LEN);
        chars_input = 0;

        while (chars_input < BODY_LEN && ((input_buf[chars_input++] = fgetc(stdin)) != '\n')) {
            if (errno == EINTR) {
                wrap_up();
            }
        }

        if (chars_input >= BODY_LEN) {
            printf("your message has been capped at 1024 bytes, please enter a shorter message next time\n\n");
            while ((c = getchar()) != '\n' && c != EOF);

            input_buf[BODY_LEN - strnlen(args.username, MAX_USERNAME_LEN) - 3 - 2] = '\n';
            input_buf[BODY_LEN - strnlen(args.username, MAX_USERNAME_LEN) - 3 - 1] = 0x0;
        }

        if (transform_input(input_buf)) {
            wrap_up();
        }

        int body_len = strnlen(input_buf, BODY_LEN) - 1;
        input_buf[body_len] = '\n';
        input_buf[body_len + 1] = 0x0;
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
