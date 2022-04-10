#include "network.h"
static int init_game_socket(void);
static int init_tcp_connection(void);
static int get_id_from_server(void);

init_connection()
{
    clientInfo = calloc(1, sizeof(client_info));
    init_tcp_connection();
    init_game_socket();
    get_id_from_server();
}
int init_game_socket()
{
    SOCKET temp;
    struct addrinfo *peer_address;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo("127.0.0.1", "4983", &hints, &peer_address))
    {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
                sizeof(address_buffer), service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);

    temp = socket(peer_address->ai_family, peer_address->ai_socktype,
                         peer_address->ai_protocol);
    if (!ISVALIDSOCKET(temp))
    {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    clientInfo->udp_socket = temp;
    return 0;
}



int init_tcp_connection()
{
    struct addrinfo *peer_address;
    struct addrinfo hints;
    SOCKET temp;
    printf("Configuring remote address...\n");
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("127.0.0.1", "7523", &hints, &peer_address))
    {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
                sizeof(address_buffer), service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);
    printf("Creating socket...\n");

    temp = socket(peer_address->ai_family, peer_address->ai_socktype,
                         peer_address->ai_protocol);
    if (!ISVALIDSOCKET(temp))
    {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    clientInfo->tcp_socket = temp;
    printf("temp %d\n", temp);
    printf("Connecting...\n");
    if (connect(clientInfo->tcp_socket, peer_address->ai_addr, peer_address->ai_addrlen))
    {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);
    return 0;
}
int get_id_from_server()
{
    char id_buffer[BUFFER_ID];
    memset(id_buffer, 0, BUFFER_ID);
    uint16_t temp_id;
    ssize_t message_length;
    message_length = read(clientInfo->tcp_socket, id_buffer,
                          BUFFER_ID);
    if (message_length < 0)
    {
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,
                       SDL_LOG_PRIORITY_INFO,
                       "could not get an ID");
        return 1;
    }
    temp_id = (uint16_t)(id_buffer[0] | (uint16_t)id_buffer[1] << 8);
    clientInfo->client_id = temp_id;

    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,
                   SDL_LOG_PRIORITY_INFO,
                   "ID : %d", clientInfo->client_id);
    return 0;
}
int  receive_udp_packet()
{
    uint8_t header[12] = {0};
    ssize_t header_size = 12;
    ssize_t entity_packet_size = 6;
    ssize_t bullet_packet_size = 4;
    ssize_t count;
    count = recvfrom(clientInfo->udp_socket, header, (size_t)header_size,
                     MSG_PEEK | MSG_DONTWAIT, NULL, NULL);
    if (count < 12 || errno == EAGAIN)
    {
        return 1;
    }

    uint64_t packet_no =
            (uint64_t)((uint64_t)header[7] | (uint64_t)header[6] << 8 |
                       (uint64_t)header[5] << 16 | (uint64_t)header[4] << 24 |
                       (uint64_t)header[3] << 32 | (uint64_t)header[2] << 40 |
                       (uint64_t)header[1] << 48 | (uint64_t)header[0] << 56);
    uint16_t num_entities = (uint16_t)(header[8] | (uint16_t)header[9] << 8);
    uint16_t num_bullets = (uint16_t)(header[10] | (uint16_t)header[11] << 8);
    ssize_t buffer_size = num_entities * entity_packet_size +
                          num_bullets * bullet_packet_size + header_size;


    uint8_t *buffer = calloc(1, (size_t)buffer_size);
    count = recvfrom(clientInfo->udp_socket, buffer, (size_t)buffer_size,
                     MSG_WAITALL, NULL, NULL);

    if (count < buffer_size)
    {
        free(buffer);
        printf("count not full %zd < %zd", count, buffer_size);
        return 1;
    }

    if (packet_no <= clientInfo->last_packet_received)
    {
        free(buffer);
        return 1;
    }
    return 0;
}