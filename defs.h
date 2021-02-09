#ifndef __DEFS_H__
#define __DEFS_H__

#define SEGMENT_LEN 1024
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 32
#define HEADER_LEN (MAX_USERNAME_LEN + MAX_PASSWORD_LEN)
#define BODY_LEN (SEGMENT_LEN - HEADER_LEN)

struct header_t {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
};

struct segment_t {
    struct header_t header;
    char body[SEGMENT_LEN - HEADER_LEN];
} segment;

#endif
