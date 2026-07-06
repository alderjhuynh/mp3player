#ifndef SCREEN_H
#define SCREEN_H

#include <SDL.h>
#include "font.h"
#include "player.h"
#include "playlist.h"
#include "ui.h"

typedef enum {
    SCREEN_NOW_PLAYING,
    SCREEN_LIBRARY,
} ScreenMode;

typedef struct {
    ScreenMode mode;
    Playlist playlist;
    int selected_index; /* which track is highlighted in the library view */
} ScreenState;

void screen_init(ScreenState *screen, const char *mp3_dir);

/* Routes a click-wheel hit according to which view is active:
 *   - WHEEL_HIT_MENU always toggles between the two views.
 *   - In the now-playing view, reverse/forward/playpause/select keep their
 *     normal transport meaning (see ui_handle_wheel_hit).
 *   - In the library view, reverse = navigate up, forward = navigate down,
 *     playpause = select (loads the highlighted track and switches back to
 *     the now-playing view).
 */
void screen_handle_wheel_hit(ScreenState *screen, WheelHit hit, PlayerState *player);

/* Draws whichever view is currently active inside screen.png's bounds.
 * progress ball is drawn separately by ui_draw_progress_ball and stays
 * relevant only to the now-playing view. */
void screen_draw(const ScreenState *screen, const PlayerState *player,
                  const Font *font, SDL_Renderer *renderer);

#endif /* SCREEN_H */