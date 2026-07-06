#ifndef UI_H
#define UI_H

#include <SDL.h>
#include <stdbool.h>
#include "player.h"
#include "layers.h"

typedef enum {
    WHEEL_HIT_NONE,
    WHEEL_HIT_MENU,
    WHEEL_HIT_REVERSE,
    WHEEL_HIT_FORWARD,
    WHEEL_HIT_PLAYPAUSE,
    WHEEL_HIT_SELECT, /* center disc click */
} WheelHit;

/* x,y are in canvas space (500x900), already converted from window
 * coordinates if the window is scaled. */
WheelHit ui_hit_test_wheel(int x, int y);

/* Dispatches a wheel hit to the corresponding player action, using the
 * "now playing" transport meaning (reverse/forward seek, playpause toggles,
 * select is a no-op placeholder). screen.c intercepts MENU itself and calls
 * this only for the other four hits, and only while in the now-playing
 * view see screen_handle_wheel_hit(). */
void ui_handle_wheel_hit(WheelHit hit, PlayerState *player);

/* Computes where progress_ball.png's top-left corner should be drawn for a
 * given 0.0-1.0 progress fraction, and draws it. */
void ui_draw_progress_ball(const LayerStack *stack, SDL_Renderer *renderer, double fraction);

/* True if (x,y) falls within the progress ball's (padded) grab area --
 * used on mouse-down to decide whether to start a scrub drag. */
bool ui_hit_test_progress_track(int x, int y);

/* Maps an x coordinate (canvas space) to a clamped 0.0-1.0 fraction along
 * the progress track, for use while scrubbing. */
double ui_progress_fraction_from_x(int x);

/* Per-pixel drag region: loads a CPU-side copy of case_color.png so
 * ui_point_in_case_body() can hit-test against its actual opaque shape
 * (rather than a hand-picked rectangle) to decide what counts as "grab the
 * case to move the window". Call ui_load_case_mask() once at startup and
 * ui_free_case_mask() at shutdown. */
int ui_load_case_mask(const char *assets_dir);
void ui_free_case_mask(void);
bool ui_point_in_case_body(int x, int y);

#endif /* UI_H */
