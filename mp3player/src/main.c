#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>
#include "config.h"
#include "layers.h"
#include "player.h"
#include "ui.h"
#include "font.h"
#include "screen.h"

#ifdef __APPLE__
#include "cocoa_shim.h"
#include <mach-o/dyld.h>
#endif

/* All asset paths in this codebase are relative. Sorry. */
static void chdir_to_executable_dir(void) {
#ifdef __APPLE__
    char exe_path[PATH_MAX];
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) != 0) {
        fprintf(stderr, "Warning: could not determine executable path "
                        "(buffer too small); relying on cwd for assets.\n");
        return;
    }

    char resolved[PATH_MAX];
    if (!realpath(exe_path, resolved)) {
        fprintf(stderr, "Warning: realpath('%s') failed: %s; relying on cwd "
                        "for assets.\n", exe_path, strerror(errno));
        return;
    }

    char *dir = dirname(resolved); /* may modify 'resolved' in place */
    if (chdir(dir) != 0) {
        fprintf(stderr, "Warning: chdir('%s') failed: %s; relying on cwd "
                        "for assets.\n", dir, strerror(errno));
    }
#endif
}

/* SDL_RenderWindowToLogical() assumes incoming mouse-event coordinates are
 * in "window points" (the same units as SDL_GetWindowSize()) and scales them
 * up to the renderer's logical size to compensate for HiDPI. That's true for
 * classic SDL2. But current Homebrew's "sdl2" package no longer ships real
 * SDL2; it installs sdl2-compat, a shim that forwards the SDL2 API onto an
 * actual SDL3 runtime. On a Retina display, SDL3 hands mouse events
 * to us already in physical-pixel coordinates (matching CANVAS_W x
 * CANVAS_H), not window points. Running that pixel-space coordinate through
 * SDL_RenderWindowToLogical() double-applies the 2x Retina scale, so clicks
 * land far outside the 500x900 canvas and every hit-test misses. */
static void translate_mouse_to_logical(SDL_Renderer *renderer, SDL_Window *window,
                                       int raw_x, int raw_y, int *out_x, int *out_y) {
    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);

    int out_w, out_h;
    SDL_GetRendererOutputSize(renderer, &out_w, &out_h);

    bool hidpi_mismatch = (out_w != win_w) || (out_h != win_h);

    if (hidpi_mismatch) {
        /* Retina + sdl2-compat/SDL3: already pixel-space == logical space. */
        *out_x = raw_x;
        *out_y = raw_y;
    } else {
        /* No backing-scale mismatch: raw coordinates are genuinely in
         * window points and still need the point -> logical scale-up. */
        float lx, ly;
        SDL_RenderWindowToLogical(renderer, raw_x, raw_y, &lx, &ly);
        *out_x = (int)lx;
        *out_y = (int)ly;
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    chdir_to_executable_dir();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    /* Borderless. Not resizable either, since the whole UI is a fixed-layout pixel-art
     * skin; the window is dragged by clicking its own body instead (see
     * the case-mask hit test below), since there's no titlebar to drag by
     * anymore. */
    SDL_Window *window = SDL_CreateWindow(
        "mp3 player",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        CANVAS_W / 2, CANVAS_H / 2,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, CANVAS_W, CANVAS_H);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    {
        SDL_RendererInfo info;
        if (SDL_GetRendererInfo(renderer, &info) == 0) {
            fprintf(stderr, "main: renderer backend = '%s'\n", info.name);
        }
        fprintf(stderr, "main: video driver = '%s'\n", SDL_GetCurrentVideoDriver());
    }

#ifdef __APPLE__
    /* On real SDL2, the Metal-backed CALayer is attached as soon as
     * SDL_CreateRenderer() returns, so the shim could safely run right
     * here. Under sdl2-compat, diagnostics showed that layer doesn't exist 
     * yet at this point; it only gets attached once the window has
     * actually gone through a real display cycle, which a single throwaway
     * clear+present before the run loop has even started doesn't guarantee.
     * Rather than guess how many throwaway frames are enough (and have it
     * silently start failing again the next time SDL's timing shifts),
     * cocoa_shim_make_borderless_transparent() is safe to call repeatedly,
     * so keep retrying it once per real frame, from inside the main loop
     * below, until it actually reports finding a CAMetalLayer. See
     * cocoa_shim_pending below. */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderPresent(renderer);

    cocoa_shim_make_borderless_transparent(window);
#endif

    LayerStack stack;
    if (layers_load(&stack, renderer, "assets/visual") != 0) {
        fprintf(stderr, "Could not load assets. Run this binary from the "
                        "project root so 'assets/visual' resolves.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if (ui_load_case_mask("assets/visual") != 0) {
        fprintf(stderr, "Warning: case mask failed to load -- clicking the "
                        "device body won't drag the window.\n");
    }

    Font font;
    if (font_load(&font, renderer, "assets/fonts") != 0) {
        fprintf(stderr, "Could not load font atlases from 'assets/fonts'.\n");
        ui_free_case_mask();
        layers_destroy(&stack);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    ScreenState screen;
    screen_init(&screen, MP3_LIBRARY_DIR);

    PlayerState player;
    player_init(&player);
    /* Auto-load (but don't auto-play) the first track in the library, if
     * any, so the now-playing screen and progress ball have something real
     * to show immediately instead of starting on an empty/placeholder
     * state. Selecting a different track from the library view
     *  loads + starts playback of whichever one you pick. */
    if (screen.playlist.count > 0) {
        player_load_track(&player, screen.playlist.tracks[0].path,
                           screen.playlist.tracks[0].title);
    }

    bool running = true;
    bool scrubbing = false;      /* dragging the progress ball */
    bool dragging_window = false; /* dragging the case body to move the window */
    int drag_start_global_x = 0, drag_start_global_y = 0;
    int drag_start_win_x = 0, drag_start_win_y = 0;

#ifdef __APPLE__
    /* Keep retrying the transparency shim once per real frame until it
     * actually finds and clears a CAMetalLayer (see the comment above the
     * pre-loop call). give up after a couple hundred frames. */
    bool cocoa_shim_pending = true;
    int cocoa_shim_attempts_left = 240;
#endif

    Uint64 last_ticks = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int lx, ly;
                translate_mouse_to_logical(renderer, window, event.button.x, event.button.y, &lx, &ly);

                bool hit_progress = (screen.mode == SCREEN_NOW_PLAYING) &&
                                     ui_hit_test_progress_track(lx, ly);
                WheelHit dbg_hit = ui_hit_test_wheel(lx, ly);
                fprintf(stderr, "main: click raw=(%d,%d) logical=(%d,%d) "
                                "progress_hit=%d wheel_hit=%d case_body=%d\n",
                        event.button.x, event.button.y, lx, ly,
                        hit_progress, (int)dbg_hit, ui_point_in_case_body(lx, ly));

                if (hit_progress) {
                    scrubbing = true;
                    SDL_CaptureMouse(SDL_TRUE);
                    player_seek_to_fraction(&player, ui_progress_fraction_from_x(lx));
                } else {
                    WheelHit hit = ui_hit_test_wheel(lx, ly);
                    if (hit != WHEEL_HIT_NONE) {
                        screen_handle_wheel_hit(&screen, hit, &player);
                    } else if (ui_point_in_case_body(lx, ly)) {
                        dragging_window = true;
                        SDL_CaptureMouse(SDL_TRUE);
                        SDL_GetGlobalMouseState(&drag_start_global_x, &drag_start_global_y);
                        SDL_GetWindowPosition(window, &drag_start_win_x, &drag_start_win_y);
                    }
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                if (scrubbing) {
                    int lx, ly;
                    translate_mouse_to_logical(renderer, window, event.motion.x, event.motion.y, &lx, &ly);
                    player_seek_to_fraction(&player, ui_progress_fraction_from_x(lx));
                } else if (dragging_window) {
                    int gx, gy;
                    SDL_GetGlobalMouseState(&gx, &gy);
                    SDL_SetWindowPosition(window,
                        drag_start_win_x + (gx - drag_start_global_x),
                        drag_start_win_y + (gy - drag_start_global_y));
                }
            } else if (event.type == SDL_MOUSEBUTTONUP &&
                       event.button.button == SDL_BUTTON_LEFT) {
                if (scrubbing || dragging_window) {
                    SDL_CaptureMouse(SDL_FALSE);
                }
                scrubbing = false;
                dragging_window = false;
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last_ticks) / (double)SDL_GetPerformanceFrequency();
        last_ticks = now;
        /* Don't let the automatic playhead fight the user's finger/mouse while
         * they're actively scrubbing. */
        if (!scrubbing) {
            player_tick(&player, dt);
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        layers_draw(&stack, renderer);
        screen_draw(&screen, &player, &font, renderer);
        if (screen.mode == SCREEN_NOW_PLAYING) {
            ui_draw_progress_ball(&stack, renderer, player_progress_fraction(&player));
        }

        SDL_RenderPresent(renderer);

#ifdef __APPLE__
        if (cocoa_shim_pending) {
            int metal_layers_found = cocoa_shim_make_borderless_transparent(window);
            cocoa_shim_attempts_left--;
            if (metal_layers_found > 0 || cocoa_shim_attempts_left <= 0) {
                cocoa_shim_pending = false;
            }
        }
#endif
    }

    player_shutdown(&player);
    font_destroy(&font);
    ui_free_case_mask();
    layers_destroy(&stack);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}