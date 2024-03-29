#include "main.h"
static void capFrameRate(long *then, float *remainder);
int main(int argc, char *argv[])
{
    long then;
    float remainder;

    memset(&app, 0, sizeof(App));
    initSDL();
    atexit(cleanup);
    then = SDL_GetTicks();

    init_connection();
    init_stage();
    while (1)
    {
        handle_gamepacket();
        doInput();
        send_input();
        prepareScene();
        draw();
        presentScene();
        capFrameRate(&then, &remainder);
    }

    return 0;
}
void capFrameRate(long *then, float *remainder)
{
    long wait, frameTime;
    wait = 16 + *remainder;
    *remainder -= (int)*remainder;
    frameTime = SDL_GetTicks() - *then;
    wait -= frameTime;
    if (wait < 1)
    {
        wait = 1;
    }
    SDL_Delay(wait);
    *remainder += 0.667;
    *then = SDL_GetTicks();
}