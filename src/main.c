#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bus.h"

#define SCALE       3
#define WIN_W       (NES_WIDTH  * SCALE)
#define WIN_H       (NES_HEIGHT * SCALE)
#define FPS         60
#define FRAME_MS    (1000 / FPS)

static void handle_key(NES *nes, SDL_Keycode key, bool down) {
    /* Jogador 1 */
    switch (key) {
    case SDLK_z:         nes_set_button(nes, 0, BTN_A,      down); break;
    case SDLK_x:         nes_set_button(nes, 0, BTN_B,      down); break;
    case SDLK_RETURN:    nes_set_button(nes, 0, BTN_START,  down); break;
    case SDLK_RSHIFT:    nes_set_button(nes, 0, BTN_SELECT, down); break;
    case SDLK_UP:        nes_set_button(nes, 0, BTN_UP,     down); break;
    case SDLK_DOWN:      nes_set_button(nes, 0, BTN_DOWN,   down); break;
    case SDLK_LEFT:      nes_set_button(nes, 0, BTN_LEFT,   down); break;
    case SDLK_RIGHT:     nes_set_button(nes, 0, BTN_RIGHT,  down); break;
    /* Jogador 2 */
    case SDLK_q:         nes_set_button(nes, 1, BTN_A,      down); break;
    case SDLK_w:         nes_set_button(nes, 1, BTN_B,      down); break;
    case SDLK_1:         nes_set_button(nes, 1, BTN_START,  down); break;
    case SDLK_2:         nes_set_button(nes, 1, BTN_SELECT, down); break;
    case SDLK_t:         nes_set_button(nes, 1, BTN_UP,     down); break;
    case SDLK_g:         nes_set_button(nes, 1, BTN_DOWN,   down); break;
    case SDLK_f:         nes_set_button(nes, 1, BTN_LEFT,   down); break;
    case SDLK_h:         nes_set_button(nes, 1, BTN_RIGHT,  down); break;
    default: break;
    }
}

static void handle_gamepad(NES *nes, int player, SDL_GameControllerButton btn, bool down) {
    switch (btn) {
    case SDL_CONTROLLER_BUTTON_A:          nes_set_button(nes, player, BTN_A,      down); break;
    case SDL_CONTROLLER_BUTTON_B:          nes_set_button(nes, player, BTN_B,      down); break;
    case SDL_CONTROLLER_BUTTON_START:      nes_set_button(nes, player, BTN_START,  down); break;
    case SDL_CONTROLLER_BUTTON_BACK:       nes_set_button(nes, player, BTN_SELECT, down); break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:    nes_set_button(nes, player, BTN_UP,     down); break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  nes_set_button(nes, player, BTN_DOWN,   down); break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  nes_set_button(nes, player, BTN_LEFT,   down); break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: nes_set_button(nes, player, BTN_RIGHT,  down); break;
    default: break;
    }
}

//Screenshot
static void save_screenshot(NES *nes, const char *prefix) {
    char name[128];
    time_t t = time(NULL);
    snprintf(name, sizeof(name), "%s_%ld.bmp", prefix, (long)t);

    SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(
        nes->ppu.fb, NES_WIDTH, NES_HEIGHT, 32,
        NES_WIDTH * 4,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_SaveBMP(surf, name);
    SDL_FreeSurface(surf);
    printf("Screenshot saved: %s\n", name);
}

/* ── Programa principal ── */
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
            "NES Emulator\n"
            "Usage: %s <rom.nes>\n\n"
            "Controls (Player 1):\n"
            "  Z/X        = A/B\n"
            "  Enter      = Start\n"
            "  RShift     = Select\n"
            "  Arrow keys = D-pad\n\n"
            "Controls (Player 2):\n"
            "  Q/W   = A/B\n"
            "  1/2   = Start/Select\n"
            "  T/G/F/H = Up/Down/Left/Right\n\n"
            "Special:\n"
            "  R = Reset\n"
            "  P = Screenshot\n"
            "  ESC = Quit\n",
            argv[0]);
        return 1;
    }

    NES *nes = nes_create(argv[1]);
    if (!nes) return 1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        nes_destroy(nes); return 1;
    }

    SDL_Window *win = SDL_CreateWindow(
        "NES Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_RenderSetLogicalSize(ren, NES_WIDTH, NES_HEIGHT);

    SDL_Texture *tex = SDL_CreateTexture(ren,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        NES_WIDTH, NES_HEIGHT);


    SDL_GameController *pads[2] = {NULL, NULL};
    for (int i = 0; i < SDL_NumJoysticks() && i < 2; i++) {
        if (SDL_IsGameController(i))
            pads[i] = SDL_GameControllerOpen(i);
    }


    bool running = true;
    Uint32 last_time = SDL_GetTicks();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN:
                if (ev.key.keysym.sym == SDLK_ESCAPE) { running = false; break; }
                if (ev.key.keysym.sym == SDLK_r)      { nes_reset(nes); break; }
                if (ev.key.keysym.sym == SDLK_p)      { save_screenshot(nes, "nes_screenshot"); break; }
                handle_key(nes, ev.key.keysym.sym, true);
                break;
            case SDL_KEYUP:
                handle_key(nes, ev.key.keysym.sym, false);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                for (int p = 0; p < 2; p++)
                    if (pads[p] && SDL_GameControllerGetJoystick(pads[p]) ==
                        SDL_JoystickFromInstanceID(ev.cbutton.which))
                        handle_gamepad(nes, p, ev.cbutton.button, true);
                break;
            case SDL_CONTROLLERBUTTONUP:
                for (int p = 0; p < 2; p++)
                    if (pads[p] && SDL_GameControllerGetJoystick(pads[p]) ==
                        SDL_JoystickFromInstanceID(ev.cbutton.which))
                        handle_gamepad(nes, p, ev.cbutton.button, false);
                break;
            case SDL_CONTROLLERDEVICEADDED:
                for (int p = 0; p < 2; p++)
                    if (!pads[p]) { pads[p] = SDL_GameControllerOpen(ev.cdevice.which); break; }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                for (int p = 0; p < 2; p++) {
                    if (pads[p] && SDL_GameControllerGetJoystick(pads[p]) ==
                        SDL_JoystickFromInstanceID(ev.cdevice.which)) {
                        SDL_GameControllerClose(pads[p]); pads[p] = NULL;
                    }
                }
                break;
            }
        }

    
        nes_run_frame(nes);

        //Envia framebuffer para GPU
        SDL_UpdateTexture(tex, NULL, nes->ppu.fb, NES_WIDTH * sizeof(u32));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        Uint32 now    = SDL_GetTicks();
        Uint32 elapsed = now - last_time;
        if (elapsed < FRAME_MS) SDL_Delay(FRAME_MS - elapsed);
        last_time = SDL_GetTicks();

        char title[128];
        float fps_real = elapsed > 0 ? 1000.0f / elapsed : 60.0f;
        snprintf(title, sizeof(title), "NES Emulator | %.1f FPS", fps_real);
        SDL_SetWindowTitle(win, title);
    }

    for (int p = 0; p < 2; p++)
        if (pads[p]) SDL_GameControllerClose(pads[p]);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    nes_destroy(nes);
    return 0;
}
