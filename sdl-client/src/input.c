#include "input.h"
static void mapInput();
void doKeyUp(SDL_KeyboardEvent *event)
{
    if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS)
    {
        app.keyboard[event->keysym.scancode] = 0;
    }
}

void doKeyDown(SDL_KeyboardEvent *event)
{
    if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS)
    {
        app.keyboard[event->keysym.scancode] = 1;
    }
}

void doInput(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                exit(0);
                break;

            case SDL_KEYDOWN:
                doKeyDown(&event.key);
                break;

            case SDL_KEYUP:
                doKeyUp(&event.key);
                break;

            default:
                break;
        }
    }
    mapInput();
}
void mapInput(void)
{
    memset(&clientInfo->clientInputState, 0, sizeof clientInfo->clientInputState);
    if (app.keyboard[SDL_SCANCODE_S])
    {
        clientInfo->clientInputState.move_down = true;
    }
    if (app.keyboard[SDL_SCANCODE_W])
    {
        clientInfo->clientInputState.move_up = true;
    }
    if (app.keyboard[SDL_SCANCODE_A])
    {
        clientInfo->clientInputState.move_left = true;
    }

    if (app.keyboard[SDL_SCANCODE_D])
    {
        clientInfo->clientInputState.move_right = true;
    }

    if (app.keyboard[SDL_SCANCODE_UP])
    {
        clientInfo->clientInputState.shoot_up = true;
        play_bullet_sound();
    }
    if (app.keyboard[SDL_SCANCODE_DOWN])
    {
        clientInfo->clientInputState.shoot_down = true;
        play_bullet_sound();
    }
    if (app.keyboard[SDL_SCANCODE_LEFT])
    {
        clientInfo->clientInputState.shoot_left = true;
        play_bullet_sound();

    }
    if (app.keyboard[SDL_SCANCODE_RIGHT])
    {
        clientInfo->clientInputState.shoot_right= true;
        play_bullet_sound();
    }

}

void play_bullet_sound(void) {
    SDL_AudioSpec WAV_spec;
    uint32_t WAV_length;
    uint8_t *WAV_buffer;
    SDL_LoadWAV("../gfx/Sound_userShoot.wav", &WAV_spec, &WAV_buffer, &WAV_length);

    SDL_AudioDeviceID audio_device_ID = SDL_OpenAudioDevice(NULL, 0, &WAV_spec, NULL, 0);
    SDL_QueueAudio(audio_device_ID, WAV_buffer, WAV_length);
    SDL_PauseAudioDevice(audio_device_ID, 0);
}