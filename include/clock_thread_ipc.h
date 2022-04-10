//
// Created by drep on 2022-04-10.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
//#include <bits/types/sigevent_t.h> // this is not portable + no usage of sigevent, just signals
#include <signal.h>

#ifndef STIRLING_GAME_DEMO_CLOCK_H
#define STIRLING_GAME_DEMO_CLOCK_H

typedef struct {
    const char * listen_path;
    const char * clock_path;
} ipc_path_pair;

struct clock_thread_args {
    ipc_path_pair * ipcPathPair;
    struct timespec * tick_rate;
};

void * clock_thread_socket(struct clock_thread_args * clockThreadArgs);
int connect_to_unix_socket(ipc_path_pair * ipcPathPair);

int create_unix_stream_socket(const char *socket_path);
int accept_ipc_connection(int server_socket);
void read_packet_from_unix_socket(int client_socket);
#endif //STIRLING_GAME_DEMO_CLOCK_H
