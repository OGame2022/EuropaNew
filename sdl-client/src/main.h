
#include "common.h"

extern void cleanup(void);
extern void doInput(void);
extern void initSDL(void);
extern void prepareScene(void);
extern void presentScene(void);

extern void init_connection(void);
extern void handle_gamepacket(void);
extern void send_input(void);
extern void init_stage(void);
extern void draw(void);

extern Entity *player;
extern client_info *clientInfo;
gamestate state;
App app;
Stage stage;
