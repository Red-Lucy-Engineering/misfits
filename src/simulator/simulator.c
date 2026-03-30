#if 0
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <stdint.h>
#include <stdio.h>

#include "../shared/include/graphics.h"
#include "../shared/include/shell.h"
#include "../shared/include/filesystem.h"

#define SIM_SCALE      3
#define SIM_WIN_WIDTH  (GRA_SCREEN_WIDTH  * SIM_SCALE)
#define SIM_WIN_HEIGHT (GRA_SCREEN_HEIGHT * SIM_SCALE)

static uint32_t g_rgba_buf[GRA_SCREEN_WIDTH * GRA_SCREEN_HEIGHT];

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "M.A.W.S", // Misfits Accuracy Watch Simulator
        SIM_WIN_WIDTH, SIM_WIN_HEIGHT, 0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        GRA_SCREEN_WIDTH, GRA_SCREEN_HEIGHT
    );

    if (fls_init()) return 1;
    if (shl_init()) return 1;

    shl_on_minute();

    bool running    = true;
    bool dirty      = true;
    Uint64   last_min   = SDL_GetTicks();
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode k = event.key.key;
                if      (k == SDLK_ESCAPE) { running = false; }
                else if (k == SDLK_1)      { shl_on_button(0); dirty = true; }
                else if (k == SDLK_2)      { shl_on_button(1); dirty = true; }
                else if (k == SDLK_3)      { shl_on_button(2); dirty = true; }
                else if (k == SDLK_4)      { shl_on_button(3); dirty = true; }
                else if (k == SDLK_F)      { shl_on_minute(); dirty = true; }
            }
        }

        Uint64 now = SDL_GetTicks();
        if (now - last_min >= 1000 * 60) {
            last_min = now;
            shl_on_minute();
            dirty = true;
        }

        for (int i = 0; i < GRA_SCREEN_WIDTH * GRA_SCREEN_HEIGHT; i++) {
            int bit = (gra_screen_buffer[i >> 3] >> (7 - (i & 7))) & 1;
            g_rgba_buf[i] = bit ? 0x000000FF : 0x0;
        }

        if (dirty) {
            SDL_UpdateTexture(texture, NULL, g_rgba_buf, GRA_SCREEN_WIDTH * sizeof(uint32_t));
            SDL_SetRenderDrawColor(renderer, 0x12, 0x12, 0x12, 0xFF);
            dirty = false;
        }

        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        SDL_RenderFillRect(renderer, NULL);
        SDL_RenderTexture(renderer, texture, NULL, NULL);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
