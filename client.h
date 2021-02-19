#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "defs.h"

#define MOV_CURSOR_TO(x, y) printf("\033[%d;%dH", (y), (x))

#define HAPPY 0x16d2a
#define SAD 0x16d0a
#define TIME 0x397640aa
#define TIME_PLUS_ONE_HOUR 0xb6ca2e4a
#define EXIT 0xb88db28a

void *handle_input(int *fd);
int transform_input(char *s);
void *handle_recieve(int *fd);

#endif
