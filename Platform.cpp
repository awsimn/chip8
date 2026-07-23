#include "Platform.h"

Platform::Platform(char const *title, int windowWidth, int windowHeight, int textureWidth, int textureHeight)
{
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow(title, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);
}

Platform::~Platform()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Platform::Update(void const *buffer, int pitch)
{
    SDL_UpdateTexture(texture, nullptr, buffer, pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

bool Platform::ProcessInput(uint8_t *keys)
{
    bool quit = false;
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            quit = true;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                quit = true;
                break;
            case SDLK_x:
                keys[0x0] = 1;
                break;
            case SDLK_1:
                keys[0x1] = 1;
                break;
            case SDLK_2:
                keys[0x2] = 1;
                break;
            case SDLK_3:
                keys[0x3] = 1;
                break;
            case SDLK_q:
                keys[0x4] = 1;
                break;
            case SDLK_w:
                keys[0x5] = 1;
                break;
            case SDLK_e:
                keys[0x6] = 1;
                break;
            case SDLK_a:
                keys[0x7] = 1;
                break;
            case SDLK_s:
                keys[0x8] = 1;
                break;
            case SDLK_d:
                keys[0x9] = 1;
                break;
            case SDLK_z:
                keys[0xA] = 1;
                break;
            case SDLK_c:
                keys[0xB] = 1;
                break;
            case SDLK_4:
                keys[0xC] = 1;
                break;
            case SDLK_r:
                keys[0xD] = 1;
                break;
            case SDLK_f:
                keys[0xE] = 1;
                break;
            case SDLK_v:
                keys[0xF] = 1;
                break;
            }
            break;

        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
            case SDLK_x:
                keys[0x0] = 0;
                break;
            case SDLK_1:
                keys[0x1] = 0;
                break;
            case SDLK_2:
                keys[0x2] = 0;
                break;
            case SDLK_3:
                keys[0x3] = 0;
                break;
            case SDLK_q:
                keys[0x4] = 0;
                break;
            case SDLK_w:
                keys[0x5] = 0;
                break;
            case SDLK_e:
                keys[0x6] = 0;
                break;
            case SDLK_a:
                keys[0x7] = 0;
                break;
            case SDLK_s:
                keys[0x8] = 0;
                break;
            case SDLK_d:
                keys[0x9] = 0;
                break;
            case SDLK_z:
                keys[0xA] = 0;
                break;
            case SDLK_c:
                keys[0xB] = 0;
                break;
            case SDLK_4:
                keys[0xC] = 0;
                break;
            case SDLK_r:
                keys[0xD] = 0;
                break;
            case SDLK_f:
                keys[0xE] = 0;
                break;
            case SDLK_v:
                keys[0xF] = 0;
                break;
            }
            break;
        }
    }

    return quit;
}