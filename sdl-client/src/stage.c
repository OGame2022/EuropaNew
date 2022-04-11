#include "stage.h"
static void do_bullets(const uint8_t * buf);
static void do_players(const uint8_t * buf);
static void free_players(void);
static void free_bullets(void);
static void draw_bullets(void);
static void draw_players(void);
static float get_shoot_angle(client_node * node);
static int is_player(client_node * node);
static SDL_Texture  *bulletTexture;
static SDL_Texture *enemyTexture;
static SDL_Texture *playerTexture;

void init_stage(){
    bulletTexture = loadTexture("gfx/playerBullet.png");
    enemyTexture = loadTexture("gfx/enemy.png");
    playerTexture = loadTexture("gfx/player.png");
}
void process_game_state(const uint8_t * buf)
{
    free_players();
    free_bullets();
    do_players(buf+HEADER_SIZE);
    do_bullets(buf+HEADER_SIZE+(state.num_entities*6));
}

void do_players(const uint8_t * buf)
{
    size_t shift = 0;
    for (uint16_t entity_no = 0; entity_no < state.num_entities; ++entity_no)
    {
        client *new_entity     = calloc(1, sizeof(client));
        new_entity->client_id  = (uint16_t)(buf[0 + shift] | (uint16_t)buf[1 + shift] << 8);
        new_entity->position_x = (uint16_t)(buf[2 + shift] | (uint16_t)buf[3 + shift] << 8);
        new_entity->position_y = (uint16_t)(buf[4 + shift] | (uint16_t)buf[5 + shift] << 8);
        new_entity->texture    = (new_entity->client_id == clientInfo->client_id) ? playerTexture : enemyTexture;

        printf("ID: %d x: %d, y: %d\n", new_entity->client_id, new_entity->position_x, new_entity->position_y);
        shift += 6;
        client_node *clientNode         = calloc(1, sizeof(client_node));
        clientNode->client_entity       = new_entity;
        clientNode->next                = clientInfo->client_entities;
        clientInfo->client_entities     = clientNode;

    }
}

void do_bullets(const uint8_t * buf)
{
    size_t shift = 0;
    for(uint16_t entity_no = 0; entity_no < state.num_bullets; ++entity_no)
    {
        bullet *new_entity     = calloc(1, sizeof(bullet));
        new_entity->position_x = (uint16_t)(buf[0 + shift] | (uint16_t)buf[1 + shift] << 8);
        new_entity->position_y = (uint16_t)(buf[2 + shift] | (uint16_t)buf[3 + shift] << 8);
        new_entity->texture = bulletTexture;
        shift += 4;
        bullet_node *bulletNode          = calloc(1, sizeof(bullet_node));
        bulletNode->bullet               = new_entity;
        bulletNode->next                 = clientInfo->bulletList;
        clientInfo->bulletList           = bulletNode;
    }
}

void free_bullets()
{
    bullet_node *node = clientInfo->bulletList;
    while(node)
    {
        free(node->bullet);
        bullet_node *temp = node->next;
        free(node);
        node = temp;
    }
    clientInfo->bulletList = NULL;
}

void free_players()
{
    client_node *node = clientInfo->client_entities;
    while(node)
    {
        free(node->client_entity);
        client_node *temp = node->next;
        free(node);
        node = temp;
    }
    clientInfo->client_entities = NULL;
}

void draw_players()
{
    client_node *node = clientInfo->client_entities;
    while(node)
    {
        blit(node->client_entity->texture, node->client_entity->position_y * 10, node->client_entity->position_x * 10, get_shoot_angle(node));
        node = node->next;
    }
}

void draw_bullets()
{
    bullet_node  *node = clientInfo->bulletList;
    while (node)
    {
        blit(node->bullet->texture, node->bullet->position_y * 10, node->bullet->position_x * 10, 0);
        node = node->next;
    }
}
float get_shoot_angle(client_node * node)
{
    if (!is_player(node))
    {
        return 0;
    }
    float x, y, base_angle = 0;

    if (clientInfo->clientInputState.shoot_up)
    {
        y -=1;
    }
    if (clientInfo->clientInputState.shoot_down)
    {
        y+=1;
    }
    if (clientInfo->clientInputState.shoot_left)
    {
        x-=1;
        base_angle+=180;
    }
    if (clientInfo->clientInputState.shoot_right)
    {
        x+=1;
    }
    if (!x && !y)
    {
        return 0;
    }
    base_angle += atanf(y/x) * 180.0 / M_PI;
    return base_angle;
}

int is_player(client_node * node)
{
    if (node->client_entity->client_id == clientInfo->client_id)
    {
        return 1;
    }
    return 0;
}
void draw()
{
    draw_players();
    draw_bullets();
}