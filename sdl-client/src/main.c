#include "main.h"

int main(int argc, char *argv[])
{
    memset(&app, 0, sizeof(App));
    initSDL();
    atexit(cleanup);
    while (1)
    {
        prepareScene();
    }

    return 0;
}
