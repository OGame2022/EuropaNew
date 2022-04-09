typedef struct Entity Entity;

typedef struct
{
    SDL_Renderer *renderer;
    SDL_Window *window;
    int keyboard[MAX_KEYBOARD_KEYS];
} App;
