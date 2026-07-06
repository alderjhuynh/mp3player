#ifndef CONFIG_H
#define CONFIG_H

/*
 * All of the assets in assets/visual/ are exported on the same 500x900
 * canvas, already positioned where they belong. (to make my life easier)
 * Rendering is just: draw every layer at (0,0), in order, on top of each other.
 * No per-layer offsets are needed. 
 */
#define CANVAS_W 500
#define CANVAS_H 900

/* draw order, bottom to top. */
static const char *const LAYER_ORDER[] = {
    "case_color.png",
    "control_wheel.png",
    "menu_text.png",
    "reverse_text.png",
    "forward_text.png",
    "playpause_text.png",
    "case_outline.png",
    "earbud.png",
    "screen.png",
    "progress_ball.png",
};
#define LAYER_COUNT (sizeof(LAYER_ORDER) / sizeof(LAYER_ORDER[0]))

/*
 * clickwheel geometry, measured from control_wheel.png's opaque bounds:
 * (120,525)-(380,785) -> center (250, 655), radius 130.
 *
 * The four text labels sit at the compass points of that ring:
 *   menu_text.png       (200,565)-(300,590)  -> top
 *   reverse_text.png    (140,640)-(185,665)  -> left
 *   forward_text.png    (315,640)-(360,665)  -> right
 *   playpause_text.png  (230,730)-(270,755)  -> bottom
 *
 */
#define WHEEL_CENTER_X 250
#define WHEEL_CENTER_Y 655
#define WHEEL_OUTER_R  130
#define WHEEL_CENTER_BUTTON_R 40   /* inner disc = "select" click, i'll probably remove this at some point */

/*
 * Progress track geometry, measured from screen.png's opaque bounds
 * and progress_ball.png's resting bounds
 *
 * The ball's left edge travels from x=100 to x=380 (mirroring the 15px
 * margin it starts with against the screen's right edge), at a fixed
 * y-center of 430.
 */
#define PROGRESS_TRACK_X_START 100
#define PROGRESS_TRACK_X_END   380
#define PROGRESS_BALL_W        20
#define PROGRESS_BALL_H        20
#define PROGRESS_BALL_Y_CENTER 430

/* The ball has a taller invisible hit-box (same x range as the track,
 * extra vertical padding) for starting a scrub drag. */
#define PROGRESS_HIT_PAD_Y 10

/*
 * screen.png's opaque bounds,
 * all on-screen text  is
 * laid out inside this rect.
 *
 * Named SCREEN_RECT_* rather than the more obvious SCREEN_X/Y/W/H because
 * screen.h's own `#ifndef SCREEN_H` include guard would otherwise collide
 * with a SCREEN_H macro here and silently eat that entire header. Sucks, but
 * it is what it is
 */
#define SCREEN_RECT_X 85
#define SCREEN_RECT_Y 275
#define SCREEN_RECT_W 330
#define SCREEN_RECT_H 185

/*
 * Bitmap font atlases (assets/fonts directory): fixed-width glyph grids
 * measured directly off the exported PNGs.
 *   number_atlas.png: "1234567890", 10 cells, each 21x25, laid out left-to-
 *     right in order at x = i*21.
 *   text_atlas.png: "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26 cells, each 25x30,
 *     laid out left-to-right in order at x = i*25.
 * Only digits and uppercase A-Z exist. font_draw_text() upper-cases
 * letters and treats anything else (spaces, punctuation, lowercase-that-
 * isn't-a-letter) as blank advance, since there's no glyph for it.
 */
#define FONT_DIGIT_CELL_W 20
#define FONT_DIGIT_CELL_H 25
#define FONT_DIGIT_MARGIN_X 5
#define FONT_DIGIT_GLYPH_ORDER "1234567890"

/* text_atlas.png: */
#define FONT_LETTER_CELL_W 25
#define FONT_LETTER_CELL_H 30

/* elipses.png (yes bro i typed it wrong calm down I am NOT fixing that) */
#define FONT_ELLIPSIS_CELL_W 30
#define FONT_ELLIPSIS_CELL_H 30

/* Where the mp3 files live; playlist.c scans this directory for *.mp3. */
#define MP3_LIBRARY_DIR "assets/audio/mp3"
#define PLAYLIST_MAX_TRACKS 64
#define PLAYLIST_TITLE_MAX 256

/* UI sound effects (unused). */
#define SFX_DIR "assets/audio/sfx"
#define SFX_UI_CLICK SFX_DIR "/electronic_ui_click.mp3"

/* Placeholder duration used for any track picked from the library, since
 * real mp3 duration probing isn't wired in yet (see player.h). */
#define PLAYLIST_PLACEHOLDER_DURATION 180.0

/*
 * on-screen text layout, all measured against SCREEN_RECT_X/Y/W/H above.
 * "Now playing" view: title (always drawn at a fixed size -- titles that
 * don't fit get truncated with an ellipsis glyph instead of shrinking), a
 * play/pause status word, and an elapsed/duration line, stacked top-down.
 */
#define NOWPLAYING_TITLE_H       26
#define NOWPLAYING_STATUS_H      14
#define NOWPLAYING_TIME_H        15
#define NOWPLAYING_TOP_PAD       14
#define NOWPLAYING_LINE_GAP      10

/* "Library" (song list) view: a header, then a scrolling list of rows,
 * each row hard-truncated to fit, with the selected row highlighted. */
#define LIBRARY_HEADER_H        20
#define LIBRARY_HEADER_TOP_PAD  8
#define LIBRARY_LIST_TOP_GAP    10
#define LIBRARY_ROW_H           22
#define LIBRARY_ROW_TEXT_H      14
#define LIBRARY_ROW_LEFT_PAD    8
#define LIBRARY_SIDE_MARGIN     10

#endif /* CONFIG_H */