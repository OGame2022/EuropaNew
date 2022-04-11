#include "stage.h"
static void do_bullets(const uint8_t * buf);
static void do_players(const uint8_t * buf);
static void free_players(void);
static void free_bullets(void);

static SDL_Texture  *bulletTexture;
static SDL_Texture *enemyTexture;
static SDL_Texture *playerTexture;

void init_stage(){
    bulletTexture = loadTexture("gfx/playerBullet.png");
    enemyTexture = loadTexture("gfx/enemy.png");
    enemyTexture = loadTexture("gfx/player.png");
}
void process_game_state(const uint8_t * buf)
{
    free_players();
    free_bullets();
    do_players(buf);
    do_bullets(buf);
}

void do_players(const uint8_t * buf)
{
    size_t shift = 0;
    for (uint16_t entity_no = 0; entity_no < state.num_entities; ++entity_no)
    {
        client *new_entity     = calloc(1, sizeof(client));
        new_entity->client_id  = (uint16_t)(buf[0 + shift] | (uint16_t)buf[1 + shift] << 8);
        if (new_entity->client_id == clientInfo->client_id)
        {
            new_entity->texture = playerTexture;
        }
        new_entity->position_x = (uint16_t)(buf[2 + shift] | (uint16_t)buf[3 + shift] << 8);
        new_entity->position_y = (uint16_t)(buf[4 + shift] | (uint16_t)buf[5 + shift] << 8);

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
    printf("players freed\n");
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