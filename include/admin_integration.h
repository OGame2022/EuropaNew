//
// Created by drep on 2022-04-01.
//

#ifndef STIRLING_GAME_DEMO_ADMIN_H
#define STIRLING_GAME_DEMO_ADMIN_H

#include <stdint.h>

#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/defaults.h>
#include <dc_application/environment.h>
#include <dc_application/options.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
//includes from tutorial
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <dc_posix/dc_netdb.h>
#include <dc_posix/sys/dc_socket.h>
#include <dc_posix/dc_signal.h>
#include <dc_posix/dc_unistd.h>
#include <time.h>
#include <dc_posix/dc_fcntl.h>

#include "common.h"

#define ADMIN_HEADER_SIZE 6
#define MAX_ADMIN_CLIENTS 15
#define ADMIN_PROTOCOL_VERSION 1
#define MAX_BUFFER_SIZE 1024

typedef struct {
    uint8_t version;
    uint8_t command;
    uint16_t target_client_id;
    uint16_t message_length;
    char * message;
} admin_client_packet;

enum ADMIN_COMMANDS {
    STOP = 1,
    USERS,
    KICK,
    WARN,
    NOTICE,
    NOT_RECOGNIZED
};

typedef struct {
    uint16_t id;
    int tcp_socket;
} admin_client;

typedef struct {
    int admin_server_socket;
    admin_client * adminClientList[MAX_ADMIN_CLIENTS];
} admin_server_info;

void admin_readPacketFromSocket(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo,
                           uint16_t admin_id, admin_client_packet *adminClientPacket, int admin_socket);
char * write_user_list_to_string(const struct dc_posix_env *env, struct dc_error *err, server_info *serverInfo);
void admin_receiveTcpPacket(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo, server_info *serverInfo, uint16_t admin_id, volatile sig_atomic_t * exit_flag);

void admin_acceptTCPConnection(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo);

void admin_addToClientList(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo, int admin_tcp_socket);
void admin_removeFromClientList(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo, uint16_t admin_id);
admin_client_packet * create_client_packet(const struct dc_posix_env *env, struct dc_error *err, enum ADMIN_COMMANDS command, char *message);
int serialize_client_packet(const struct dc_posix_env *env, struct dc_error *err, admin_client_packet * clientPacket, uint8_t **output_buffer, size_t *size);
void send_admin_client_message(const struct dc_posix_env *env, struct dc_error *err, enum ADMIN_COMMANDS command, char *message, int tcp_socket);

#endif //STIRLING_GAME_DEMO_ADMIN_H
