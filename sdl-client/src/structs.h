

#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>

typedef struct Entity Entity;

typedef struct
{
    void (*logic)(void);
    void (*draw)(void);
} Delegate;

typedef struct
{
    SDL_Renderer *renderer;
    SDL_Window *window;
    Delegate delegate;
    int keyboard[MAX_KEYBOARD_KEYS];
    int keysend[MAX_KEYS];
    int id;
} App;

typedef struct {
    bool move_up;
    bool move_down;
    bool move_left;
    bool move_right;

    bool shoot_up;
    bool shoot_down;
    bool shoot_left;
    bool shoot_right;
} client_input_state;

typedef struct {
    uint16_t client_id;
    uint16_t position_x;
    uint16_t position_y;
    client_input_state inputState;
    SDL_Texture *texture;
} client;

typedef struct {
    uint16_t shooters_id;
    uint16_t position_x;
    uint16_t position_y;
    short direction_x;
    short direction_y;
    SDL_Texture *texture;
} bullet;

typedef struct client_node {
    client * client_entity;
    struct client_node * next;
} client_node;

typedef struct bullet_node {
    bullet * bullet;
    struct bullet_node * next;
} bullet_node;

typedef struct {
    client_node * client_entities;
    uint16_t client_id;
    uint64_t last_packet_sent;
    uint64_t last_packet_received;
    int tcp_socket;
    int udp_socket;
    struct sockaddr_in * udp_address;
    socklen_t udp_adr_len;
    bool has_client_entity;
    client * client_entity;
    client_input_state clientInputState;
    bullet_node * bulletList;
} client_info;

struct Entity
{
    float x;
    float y;
    int w;
    int h;
    float dx;
    float dy;
    int side;
    int health;
    int reload;
    SDL_Texture *texture;
    Entity *next;
};

typedef struct
{
    Entity fighterHead, *fighterTail;
    Entity bulletHead, *bulletTail;
} Stage;

typedef struct
{
    uint16_t num_entities;
    uint16_t num_bullets;

} gamestate;