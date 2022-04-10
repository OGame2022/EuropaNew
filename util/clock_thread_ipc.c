//
// Created by drep on 2022-04-10.
//

#include "clock_thread_ipc.h";
void * clock_thread_socket(struct clock_thread_args * clockThreadArgs) {
    // initalize socket to talk over
    int client_socket = connect_to_unix_socket(clockThreadArgs->ipcPathPair);
    struct timespec remaining;
    //TODO: error checking

    while(1) {
        nanosleep(clockThreadArgs->tick_rate, &remaining);
        const char * buffer = "0";
        ssize_t retval;
        retval = write(client_socket, buffer, 1);
        if (retval <= 0) {
            return 0;
        }
    }
    return 0;
}

int connect_to_unix_socket(ipc_path_pair * ipcPathPair) {
    int rc;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    int client_socket;
    socklen_t client_socklen;
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));


    client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_socket == -1) {
        printf("client: SOCKET ERROR = %d\n", errno);
        exit(1);
    }

    client_sockaddr.sun_family = AF_UNIX;
    strcpy(client_sockaddr.sun_path, ipcPathPair->clock_path);
    client_socklen = sizeof(client_sockaddr);

    unlink(ipcPathPair->clock_path);
    rc = bind(client_socket, (struct sockaddr *) &client_sockaddr, client_socklen);
    if (rc == -1){
        printf("client: BIND ERROR: %d\n", errno);
        close(client_socket);
        exit(1);
    }

    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* for the server socket and connect   */
    /* to it.                              */
    /***************************************/
    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, ipcPathPair->listen_path);
    socklen_t server_socklen;
    server_socklen = sizeof(server_sockaddr);
    rc = connect(client_socket, (struct sockaddr *) &server_sockaddr, server_socklen);
    if(rc == -1){
        printf("client: CONNECT ERROR = %d\n", errno);
        close(client_socket);
        exit(1);
    }
    return client_socket;
}

int create_unix_stream_socket(const char *socket_path) {
    socklen_t server_socklen;
    int server_socket;
    struct sockaddr_un server_sockaddr;
    int backlog = 10;

    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1){
        printf("SOCKET ERROR: %d\n", errno);
        exit(1);
    }

    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, socket_path);
    server_socklen = sizeof(server_sockaddr);
    int return_value;
    unlink(socket_path);
    return_value = bind(server_socket, (struct sockaddr *) &server_sockaddr, server_socklen);
    if (return_value == -1){
        printf("BIND ERROR: %d\n", errno);
        close(server_socket);
        exit(1);
    }

    /*********************************/
    /* Listen for any client sockets */
    /*********************************/
    return_value = listen(server_socket, backlog);
    if (return_value == -1){
        printf("LISTEN ERROR: %d\n", errno);
        close(server_socket);
        exit(1);
    }
    printf("socket listening...\n");

    return server_socket;
}


int accept_ipc_connection(int server_socket) {
    /*********************************/
    /* Accept an incoming connection */
    /*********************************/
    int client_socket;
    socklen_t client_socklen;
    struct sockaddr_un client_sockaddr;

    client_socket = accept(server_socket, (struct sockaddr *) &client_sockaddr, &client_socklen);
    if (client_socket == -1){
        close(server_socket);
        close(client_socket);
    }

    /****************************************/
    /* Get the name of the connected socket */
    /****************************************/
    int return_value;
    client_socklen = sizeof(client_sockaddr);
    return_value = getpeername(client_socket, (struct sockaddr *) &client_sockaddr, &client_socklen);
    if (return_value == -1){
        close(server_socket);
        close(client_socket);
    }
    else {
        printf("Client socket filepath: %s\n", client_sockaddr.sun_path);
    }
    return client_socket;
}

void read_packet_from_unix_socket(int client_socket) {
    ssize_t bytes_rec;
    char buf[256];

    /************************************/
    /* Read and print the data          */
    /* incoming on the connected socket */
    /************************************/
    //printf("waiting to read...\n");
    bytes_rec = recv(client_socket, buf, sizeof(buf), 0);
    if (bytes_rec == -1){
        //printf("RECV ERROR: %d\n", sock_errno());
        close(client_socket);
        exit(1);
    } else if (bytes_rec == 0) {
        printf("server: client disconnected\n");
        close(client_socket);
        exit(1);
    }
    else {
        //printf("server: DATA RECEIVED = %s\n", buf);
    }
}
