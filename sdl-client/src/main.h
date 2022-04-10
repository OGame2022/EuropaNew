
#include "common.h"

extern void cleanup(void);
extern void doInput(void);
extern void initSDL(void);
extern void initStage(void);
extern void prepareScene(void);
extern void presentScene(void);

extern int send_message(App app);
extern int init_connection(void);
extern int networkThread(void *ptr);
extern int receive_udp_packet(void);
extern Entity *player;
App app;
Stage stage;
