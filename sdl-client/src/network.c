#include "network.h"
static int init_game_socket(void);
static int init_tcp_connection(void);
static int get_id_from_server(void);
static int receive_udp_packet(char * buffer);

void init_connection()
{
    clientInfo = calloc(1, sizeof(client_info));
    init_tcp_connection();
    init_game_socket();
    get_id_from_server();
}

void handle_gamepacket(void)
{
    uint8_t gamestate_buf[4096];
    memset(gamestate_buf, 0, 4096);
    while(receive_udp_packet(gamestate_buf) != 1)
        ;

    if (gamestate_buf != NULL)
    {
        process_game_state(gamestate_buf);
    }
    printf("num bullies %d\n", state.num_bullets);
}

void send_input()
{
    uint8_t packet[11];
    memset(packet, 0 , 11);
    uint64_t packet_no = ++ clientInfo->last_packet_sent;

    for(int i = 0; i < 8; i++) {
        packet[i] = (uint8_t) ((packet_no >> 8 * (7 - i)) & 0xFF);
    }
    packet[8] = clientInfo->client_id & 0xFF;
    packet[9] = clientInfo-> client_id >> 8;

    if(clientInfo->client_entity)
    {
        uint8_t packed_byte = 0;
        packed_byte |= (clientInfo->clientInputState.move_up << 0);
        packed_byte |= (clientInfo->clientInputState.move_down << 1);
        packed_byte |= (clientInfo->clientInputState.move_left << 2);
        packed_byte |= (clientInfo->clientInputState.move_right << 3);
        packed_byte |= (clientInfo->clientInputState.shoot_up << 4);
        packed_byte |= (clientInfo->clientInputState.shoot_down << 5);
        packed_byte |= (clientInfo->clientInputState.shoot_left << 6);
        packed_byte |= (clientInfo->clientInputState.shoot_right << 7);
        packet[10] = packed_byte;
    }

    if(sendto(clientInfo->udp_socket, packet, 11, 0,
              peer_address->ai_addr, peer_address->ai_addrlen ) < 0)
    {
        printf("Unable to send message\n");
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

}

int init_game_socket()
{
    SOCKET temp;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo("127.0.0.1", "8528", &hints, &peer_address))
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
    clientInfo->has_client_entity = true;
    clientInfo->client_entity = calloc(1,sizeof (client));
    return 0;
}



int init_tcp_connection()
{
    struct addrinfo *peer_address_tcp;
    struct addrinfo hints;
    SOCKET temp;
    printf("Configuring remote address...\n");
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("127.0.0.1", "7528", &hints, &peer_address_tcp))
    {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address_tcp->ai_addr, peer_address_tcp->ai_addrlen, address_buffer,
                sizeof(address_buffer), service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);
    printf("Creating socket...\n");

    temp = socket(peer_address_tcp->ai_family, peer_address_tcp->ai_socktype,
                  peer_address_tcp->ai_protocol);
    if (!ISVALIDSOCKET(temp))
    {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    clientInfo->tcp_socket = temp;
    printf("temp %d\n", temp);
    printf("Connecting...\n");
    if (connect(clientInfo->tcp_socket, peer_address_tcp->ai_addr, peer_address_tcp->ai_addrlen))
    {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address_tcp);
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
int  receive_udp_packet(char *buffer)
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

    state.num_bullets = num_bullets;
    state.num_entities = num_entities;

    count = recvfrom(clientInfo->udp_socket, buffer, (size_t)buffer_size,
                     MSG_WAITALL, NULL, NULL);

    if (count < buffer_size)
    {
        memset(buffer, 0 , 4096);
        printf("count not full %zd < %zd", count, buffer_size);
        return 1;
    }

    if (packet_no <= clientInfo->last_packet_received)
    {
        memset(buffer, 0 , 4096);
        return 1;
    }
    printf("%zd\n", count);
    return 0;
}
