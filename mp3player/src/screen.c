#include "screen.h"
#include "config.h"
#include "id3.h"
#include <stdio.h>
#include <string.h>

static const int SCREEN_MARGIN = 10;

void screen_init(ScreenState *screen, const char *mp3_dir) {
    memset(screen, 0, sizeof(*screen));
    screen->mode = SCREEN_NOW_PLAYING;
    screen->selected_index = 0;
    playlist_load(&screen->playlist, mp3_dir);
}

void screen_handle_wheel_hit(ScreenState *screen, WheelHit hit, PlayerState *player) {
    if (hit == WHEEL_HIT_MENU) {
        screen->mode = (screen->mode == SCREEN_NOW_PLAYING) ? SCREEN_LIBRARY : SCREEN_NOW_PLAYING;
        return;
    }

    if (screen->mode == SCREEN_NOW_PLAYING) {
        /* Normal transport controls, unchanged. */
        ui_handle_wheel_hit(hit, player);
        return;
    }

    /* Library view: reverse/forward browse, playpause/select loads the
     * highlighted track and jumps back to the now-playing view. */
    int count = screen->playlist.count;
    if (count == 0) return;

    switch (hit) {
        case WHEEL_HIT_REVERSE: /* navigate up */
            screen->selected_index = (screen->selected_index - 1 + count) % count;
            break;
        case WHEEL_HIT_FORWARD: /* navigate down */
            screen->selected_index = (screen->selected_index + 1) % count;
            break;
        case WHEEL_HIT_PLAYPAUSE:
        case WHEEL_HIT_SELECT: {
            const Track *t = &screen->playlist.tracks[screen->selected_index];
            if (player_load_track(player, t->path, t->title)) {
                player_play(player);
                player_play_sfx(player, SFX_UI_CLICK);
            }
            screen->mode = SCREEN_NOW_PLAYING;
            break;
        }
        default:
            break;
    }
}

static void format_time(double seconds, char *out, size_t out_size) {
    if (seconds < 0.0) seconds = 0.0;
    int total = (int)(seconds + 0.5);
    int mins = total / 60;
    int secs = total % 60;
    snprintf(out, out_size, "%d:%02d", mins, secs);
}

static void draw_now_playing(const ScreenState *screen, const PlayerState *player,
                              const Font *font, SDL_Renderer *renderer) {
    (void)screen;
    int x0 = SCREEN_RECT_X + SCREEN_MARGIN;
    int w = SCREEN_RECT_W - 2 * SCREEN_MARGIN;
    int y = SCREEN_RECT_Y + NOWPLAYING_TOP_PAD;

    const char *title = player->has_track ? player->title : "NO TRACK LOADED";
    font_draw_title(font, renderer, title, x0, w, y, NOWPLAYING_TITLE_H);
    y += NOWPLAYING_TITLE_H + NOWPLAYING_LINE_GAP;

    const char *status = !player->has_track ? "NO TRACK"
                        : player->is_playing ? "PLAYING"
                                              : "PAUSED";
    font_draw_text_centered(font, renderer, status, x0, w, y, NOWPLAYING_STATUS_H);
    y += NOWPLAYING_STATUS_H + NOWPLAYING_LINE_GAP;

    if (player->has_track) {
        char elapsed[16], duration[16], line[40];
        format_time(player->elapsed_seconds, elapsed, sizeof(elapsed));
        format_time(player->duration_seconds, duration, sizeof(duration));
        /* '/' has no glyph in the font, so it renders as a blank gap
         * which doubles as a perfectly serviceable separator here. */
        snprintf(line, sizeof(line), "%s / %s", elapsed, duration);
        font_draw_text_centered(font, renderer, line, x0, w, y, NOWPLAYING_TIME_H);
    }
}

static void draw_library(const ScreenState *screen, const Font *font, SDL_Renderer *renderer) {
    int x0 = SCREEN_RECT_X + SCREEN_MARGIN;
    int w = SCREEN_RECT_W - 2 * SCREEN_MARGIN;
    int y = SCREEN_RECT_Y + LIBRARY_HEADER_TOP_PAD;

    font_draw_text_centered(font, renderer, "LIBRARY", x0, w, y, LIBRARY_HEADER_H);

    int list_top = y + LIBRARY_HEADER_H + LIBRARY_LIST_TOP_GAP;
    int list_bottom = SCREEN_RECT_Y + SCREEN_RECT_H;

    if (screen->playlist.count == 0) {
        font_draw_text_centered(font, renderer, "NO TRACKS FOUND", x0, w,
                                 list_top, LIBRARY_ROW_TEXT_H);
        return;
    }

    int visible_rows = (list_bottom - list_top) / LIBRARY_ROW_H;
    if (visible_rows < 1) visible_rows = 1;

    int offset = 0;
    if (screen->selected_index >= visible_rows) offset = screen->selected_index - visible_rows + 1;
    if (screen->selected_index < offset) offset = screen->selected_index;

    int row_x0 = SCREEN_RECT_X + LIBRARY_SIDE_MARGIN;
    int row_w = SCREEN_RECT_W - 2 * LIBRARY_SIDE_MARGIN;
    int text_max_w = row_w - LIBRARY_ROW_LEFT_PAD;

    for (int i = offset; i < screen->playlist.count && (i - offset) < visible_rows; i++) {
        int row_y = list_top + (i - offset) * LIBRARY_ROW_H;

        if (i == screen->selected_index) {
            SDL_SetRenderDrawColor(renderer, 133, 154, 232, 255);
            SDL_Rect hl = {row_x0, row_y, row_w, LIBRARY_ROW_H - 2};
            SDL_RenderFillRect(renderer, &hl);
        }

        char row_text[64];
        font_truncate_to_width(font, screen->playlist.tracks[i].title, row_text,
                                sizeof(row_text), text_max_w, LIBRARY_ROW_TEXT_H);

        int text_y = row_y + (LIBRARY_ROW_H - 2 - LIBRARY_ROW_TEXT_H) / 2;
        font_draw_text(font, renderer, row_text, row_x0 + LIBRARY_ROW_LEFT_PAD, text_y,
                        LIBRARY_ROW_TEXT_H);
    }
}

void screen_draw(const ScreenState *screen, const PlayerState *player,
                  const Font *font, SDL_Renderer *renderer) {
    if (screen->mode == SCREEN_NOW_PLAYING) {
        draw_now_playing(screen, player, font, renderer);
    } else {
        draw_library(screen, font, renderer);
    }
}
