
#include "common.h"

extern void cleanup(void);
extern void doInput(void);
extern void initSDL(void);
extern void initStage(void);
extern void prepareScene(void);
extern void presentScene(void);

extern int send_message(App app);
extern void init_connection(void);
extern int networkThread(void *ptr);
extern void handle_gamepacket(void);
extern void send_input(void);
extern void init_stage(void);
extern Entity *player;
extern client_info *clientInfo;
gamestate state;
App app;
Stage stage;
