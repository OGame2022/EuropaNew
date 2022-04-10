#include "admin_integration.h"

admin_client_packet * create_client_packet(const struct dc_posix_env *env, struct dc_error *err, enum ADMIN_COMMANDS command, char *message) {
    admin_client_packet * clientPacket = NULL;
    clientPacket = dc_calloc(env, err, 1, sizeof(admin_client_packet));

    if (dc_error_has_error(err)) {
        fprintf(stderr, "Error: \"%s\" - %s : %s : %d @ %zu\n", err->message, err->file_name, err->function_name, err->errno_code, err->line_number);
        dc_exit(env, 1);
    }

    clientPacket->version = ADMIN_PROTOCOL_VERSION;
    clientPacket->command = (uint8_t) command;
    clientPacket->target_client_id = 0;
    if (message) {
        clientPacket->message_length = (uint16_t) dc_strlen(env, message);
        clientPacket->message = dc_strdup(env, err, message);
    } else {
        clientPacket->message_length = 0;
        clientPacket->message = NULL;
    }

    return clientPacket;
}

int serialize_client_packet(const struct dc_posix_env *env, struct dc_error *err, admin_client_packet * clientPacket, uint8_t **output_buffer, size_t *size)
{
    size_t header_size = 6;
    uint8_t client_header[header_size];

    client_header[0] = clientPacket->version;
    client_header[1] = clientPacket->command;
    client_header[2] = clientPacket->target_client_id & 0xFF;
    client_header[3] = clientPacket->target_client_id >> 8;
    client_header[4] = clientPacket->message_length & 0xFF;
    client_header[5] = clientPacket->message_length >> 8;

    *output_buffer = dc_calloc(env, err, (sizeof(client_header)) + clientPacket->message_length, sizeof(uint8_t));
    if (dc_error_has_error(err)) {
        fprintf(stderr, "Error: \"%s\" - %s : %s : %d @ %zu\n", err->message, err->file_name, err->function_name, err->errno_code, err->line_number);
        dc_exit(env, 1);
    }
    dc_memcpy(env, *output_buffer, client_header, sizeof(client_header));
    dc_memcpy(env, *output_buffer + sizeof(client_header), clientPacket->message, clientPacket->message_length);

    *size = (size_t) (sizeof(client_header) + clientPacket->message_length);

    return 0;
}

void send_admin_client_message(const struct dc_posix_env *env, struct dc_error *err, enum ADMIN_COMMANDS command, char *message, int tcp_socket) {
    admin_client_packet *clientPacket;
    uint8_t *output_buffer = NULL;
    size_t packetSize = 0;

    clientPacket = create_client_packet(env, err, command, message);

    serialize_client_packet(env, err, clientPacket, &output_buffer, &packetSize);

    dc_write(env, err, tcp_socket, output_buffer, packetSize);
    dc_free(env, output_buffer, packetSize);

    if (clientPacket->message) {
        dc_free(env, clientPacket->message, dc_strlen(env, clientPacket->message) + 1);
    }
    dc_free(env, clientPacket, sizeof(admin_client_packet));
}


void admin_acceptTCPConnection(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo) {
    //accept new tcp connectionasd
    printf("new TCP connection request\n");
    int client_tcp_socket = dc_accept(env, err, adminServerInfo->admin_server_socket, NULL, NULL);
    if (dc_error_has_no_error(err)) {
        printf("connected client\n");
        admin_addToClientList(env, err, adminServerInfo, client_tcp_socket);
    } else {
        if(err->type == DC_ERROR_ERRNO && err->errno_code == EINTR) {
            dc_error_reset(err);
        }
    }
}

char * write_user_list_to_string(const struct dc_posix_env *env, struct dc_error *err, server_info *serverInfo) {
    char buffer[MAX_BUFFER_SIZE] = {0};
    char *userList;
    char clientID[MAX_BUFFER_SIZE] = {0};
    char clientAddress[MAX_BUFFER_SIZE] = {0};

    for (size_t i = 0; serverInfo->connections[i]; i++) {
        sprintf(clientID, "%hu", serverInfo->connections[i]->client_id);
        dc_strcat(env, buffer, clientID);
        dc_strcat(env, buffer, " ");
        if (serverInfo->connections[i]->udp_address) {
            inet_ntop(AF_INET, serverInfo->connections[i]->udp_address, clientAddress, INET_ADDRSTRLEN);
            dc_strcat(env, buffer, clientAddress);
        } else {
            dc_strcat(env, buffer, "N/A");
        }
        dc_strcat(env, buffer, " ");
    }
    userList = dc_strdup(env, err, buffer);

    return userList;
}

void admin_receiveTcpPacket(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo,
                         server_info *serverInfo, uint16_t admin_id, volatile sig_atomic_t * exit_flag) {
    /**
     * defining a quick protocol for admin for testing, real protocol can be implemented later.
     * byte 0 - version - uint8_t
     * byte 1 - command - uint8_t
     * byte 2 & 3 - target_client_id - uint16_t
     * byte 4 & 5 - message_length - uint16_t
     * remaining bytes, message
     *
     * version = 1;
     * commands = STOP, COUNT, KICK, WARN, NOTICE
     * ;
     */

    admin_client_packet adminClientPacket = {0};
    char * message;
    int admin_socket = adminServerInfo->adminClientList[admin_id]->tcp_socket;
    printf("new tcp event from admin: %d socket %d\n", admin_id, admin_socket);

    admin_readPacketFromSocket(env, err, adminServerInfo, admin_id, &adminClientPacket, admin_socket);

    // packet fully received, process now

    if (adminClientPacket.version != ADMIN_PROTOCOL_VERSION) {
        // wrong protocol version
        printf("wrong protocol version. needed %d, have %d\n", ADMIN_PROTOCOL_VERSION, adminClientPacket.version);
    }

    switch (adminClientPacket.command) {
        case STOP:
            printf("stop command\n");
            *exit_flag = true;
            break;
        case USERS:
            message = write_user_list_to_string(env, err, serverInfo);
            send_admin_client_message(env, err, adminClientPacket.command, message, admin_socket);
            printf("user list command\n");
            break;
        case KICK:
            printf("kick command\n");
            break;
        case WARN:
            printf("warn command\n");
            break;
        case NOTICE:
            printf("notice command\n");
            break;
        default:
            printf("invalid command %d\n", adminClientPacket.command);
            break;
    }

    if (adminClientPacket.message) {
        free(adminClientPacket.message);
    }

}

void
admin_readPacketFromSocket(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo,
                           uint16_t admin_id, admin_client_packet *adminClientPacket, int admin_socket) {
    uint8_t header[ADMIN_HEADER_SIZE] = {0};
    ssize_t count = 0;
    ssize_t total_received = 0;
    size_t remaining_bytes = ADMIN_HEADER_SIZE;

    bool readComplete = false;
    while(!readComplete && (count = dc_read(env, err, admin_socket, (header + total_received), remaining_bytes)) > 0 && dc_error_has_no_error(err))
    {
        remaining_bytes -= (size_t) count;
        total_received += count;
        if (remaining_bytes == 0) {
            readComplete = true;
        }
    }
    if (count <= 0) {
        //Somebody disconnected , get his details and print
        printf("client disconnected id: %d sd %d\n" , admin_id, admin_socket);
        admin_removeFromClientList(env, err, adminServerInfo, admin_id);
        //Close the socket and mark as 0 in list for reuse
        readComplete = true;
    }
    // parse header
    (*adminClientPacket).version = header[0];
    (*adminClientPacket).command = header[1];
    (*adminClientPacket).target_client_id =  (uint16_t) (header[2] | (uint16_t) header[3] << 8);
    (*adminClientPacket).message_length =  (uint16_t) (header[4] | (uint16_t) header[5] << 8);

    if ((*adminClientPacket).message_length > 0) {
        // read remaining message
        (*adminClientPacket).message = dc_calloc(env, err, (*adminClientPacket).message_length, sizeof(uint8_t));
        count = 0;
        total_received = 0;
        remaining_bytes = (*adminClientPacket).message_length;
        readComplete = false;
        while(!readComplete && (count = dc_read(env, err, admin_socket, ((*adminClientPacket).message + total_received), remaining_bytes)) > 0 && dc_error_has_no_error(err))
        {
            remaining_bytes -= (size_t) count;
            total_received += count;
            if (remaining_bytes == 0) {
                readComplete = true;
            }
        }
        if (count <= 0) {
            //Somebody disconnected , get his details and print
            printf("client disconnected id: %d sd %d\n" , admin_id, admin_socket);
            admin_removeFromClientList(env, err, adminServerInfo, admin_id);
            //Close the socket and mark as 0 in list for reuse
            readComplete = true;
        }
    }
}

void admin_addToClientList(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo, int admin_tcp_socket) {
    for (uint16_t client_id = 0; client_id < MAX_CLIENTS; ++client_id) {
        if (adminServerInfo->adminClientList[client_id] == NULL) {
            printf("Adding to list of sockets as %d with no %d\n" , client_id, admin_tcp_socket);
            adminServerInfo->adminClientList[client_id] = calloc(1, sizeof(connection));
            adminServerInfo->adminClientList[client_id]->id = client_id;
            adminServerInfo->adminClientList[client_id]->tcp_socket = admin_tcp_socket;
            // everything else is null since calloc
//            uint8_t buffer[2] = {client_id & 0xFF, client_id >> 8};
//            write(admin_tcp_socket, buffer, 2);
            return;
        }
    }
    close(admin_tcp_socket);
}

void admin_removeFromClientList(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo, uint16_t admin_id) {
    if (adminServerInfo->adminClientList[admin_id]) {
        close(adminServerInfo->adminClientList[admin_id]->tcp_socket);
        free(adminServerInfo->adminClientList[admin_id]);
        adminServerInfo->adminClientList[admin_id] = NULL;
    }
}
