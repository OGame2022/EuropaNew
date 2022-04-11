#include "common.h"

extern SDL_Texture *loadTexture(char * path);
extern void blit(SDL_Texture *texture, int x, int y);
extern client_info *clientInfo;
extern gamestate state;