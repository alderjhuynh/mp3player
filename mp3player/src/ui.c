#include "ui.h"
#include "config.h"
#include <SDL_image.h>
#include <math.h>
#include <stdio.h>

WheelHit ui_hit_test_wheel(int x, int y) {
    double dx = x - WHEEL_CENTER_X;
    double dy = y - WHEEL_CENTER_Y;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist > WHEEL_OUTER_R) {
        return WHEEL_HIT_NONE;
    }
    if (dist <= WHEEL_CENTER_BUTTON_R) {
        return WHEEL_HIT_SELECT;
    }

    /* Split the ring into four quadrants along the diagonals */
    if (fabs(dy) >= fabs(dx)) {
        return (dy < 0) ? WHEEL_HIT_MENU : WHEEL_HIT_PLAYPAUSE;
    } else {
        return (dx < 0) ? WHEEL_HIT_REVERSE : WHEEL_HIT_FORWARD;
    }
}

void ui_handle_wheel_hit(WheelHit hit, PlayerState *player) {
    switch (hit) {
        case WHEEL_HIT_MENU:
            /* screen_handle_wheel_hit() (screen.c) intercepts MENU before
             * it ever reaches here. it always toggles between the
             * now-playing and library views. This case is unreachable in
             * normal use; kept only so the switch stays exhaustive. */
            printf("[wheel] menu\n");
            break;
        case WHEEL_HIT_REVERSE:
            player_seek_relative(player, -10.0);
            printf("[wheel] reverse -> %.1fs\n", player->elapsed_seconds);
            break;
        case WHEEL_HIT_FORWARD:
            player_seek_relative(player, 10.0);
            printf("[wheel] forward -> %.1fs\n", player->elapsed_seconds);
            break;
        case WHEEL_HIT_PLAYPAUSE:
            player_toggle_play_pause(player);
            printf("[wheel] %s\n", player->is_playing ? "play" : "pause");
            break;
        case WHEEL_HIT_SELECT:
            /* TODO (future work): center click = confirm/select. */
            printf("[wheel] select\n");
            break;
        case WHEEL_HIT_NONE:
        default:
            break;
    }
}

void ui_draw_progress_ball(const LayerStack *stack, SDL_Renderer *renderer, double fraction) {
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;

    int track_span = PROGRESS_TRACK_X_END - PROGRESS_TRACK_X_START;
    int ball_x = PROGRESS_TRACK_X_START + (int)round(fraction * track_span);
    int ball_y = PROGRESS_BALL_Y_CENTER - PROGRESS_BALL_H / 2;

    SDL_Texture *ball_tex = layers_find(stack, "progress_ball.png");
    if (!ball_tex) return;

    SDL_Rect src = {PROGRESS_TRACK_X_START, PROGRESS_BALL_Y_CENTER - PROGRESS_BALL_H / 2,
                     PROGRESS_BALL_W, PROGRESS_BALL_H};
    SDL_Rect dst = {ball_x, ball_y, PROGRESS_BALL_W, PROGRESS_BALL_H};
    SDL_RenderCopy(renderer, ball_tex, &src, &dst);
}

bool ui_hit_test_progress_track(int x, int y) {
    int y0 = PROGRESS_BALL_Y_CENTER - PROGRESS_BALL_H / 2 - PROGRESS_HIT_PAD_Y;
    int y1 = PROGRESS_BALL_Y_CENTER + PROGRESS_BALL_H / 2 + PROGRESS_HIT_PAD_Y;
    int x0 = PROGRESS_TRACK_X_START - PROGRESS_BALL_W / 2;
    int x1 = PROGRESS_TRACK_X_END + PROGRESS_BALL_W / 2;
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

double ui_progress_fraction_from_x(int x) {
    int track_span = PROGRESS_TRACK_X_END - PROGRESS_TRACK_X_START;
    double fraction = (double)(x - PROGRESS_TRACK_X_START) / (double)track_span;
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    return fraction;
}

/* CPU-side copy of case_color.png, kept only for per-pixel drag
 * hit-testing (layers.c separately loads its own GPU texture copy for
 * actually drawing it). */
static SDL_Surface *g_case_mask = NULL;

int ui_load_case_mask(const char *assets_dir) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/case_color.png", assets_dir);

    SDL_Surface *raw = IMG_Load(path);
    if (!raw) {
        fprintf(stderr, "ui: failed to load case mask '%s': %s\n", path, IMG_GetError());
        return -1;
    }

    SDL_Surface *converted = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    if (!converted) {
        fprintf(stderr, "ui: failed to convert case mask: %s\n", SDL_GetError());
        return -1;
    }

    g_case_mask = converted;
    return 0;
}

void ui_free_case_mask(void) {
    if (g_case_mask) {
        SDL_FreeSurface(g_case_mask);
        g_case_mask = NULL;
    }
}

bool ui_point_in_case_body(int x, int y) {
    if (!g_case_mask) return false;
    if (x < 0 || y < 0 || x >= g_case_mask->w || y >= g_case_mask->h) return false;

    Uint8 *row = (Uint8 *)g_case_mask->pixels + (size_t)y * g_case_mask->pitch;
    Uint32 pixel = ((Uint32 *)row)[x];
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixel, g_case_mask->format, &r, &g, &b, &a);
    return a > 10;
}