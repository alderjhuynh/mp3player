#ifndef FONT_H
#define FONT_H

#include <SDL.h>

typedef struct {
    SDL_Texture *digits_tex; 
    SDL_Texture *letters_tex; 
    SDL_Texture *ellipsis_tex;
} Font;

int font_load(Font *font, SDL_Renderer *renderer, const char *fonts_dir);
void font_destroy(Font *font);

/* Measures the rendered pixel width of `str` if drawn with font_draw_text
 * at the given glyph height */
int font_text_width(const Font *font, const char *str, int glyph_height);

int font_draw_text(const Font *font, SDL_Renderer *renderer, const char *str,
                    int x, int y, int glyph_height);

/* Convenience: draws str horizontally centered in [x, x+w), same y/height
 * semantics as font_draw_text. */
int font_draw_text_centered(const Font *font, SDL_Renderer *renderer,
                             const char *str, int x, int w, int y,
                             int glyph_height);

/* Finds the largest glyph_height <= max_height 
 * for which str still fits within max_width. 
 * Falls back to min_height if it never fits. */
int font_fit_height(const Font *font, const char *str, int max_width,
                     int min_height, int max_height);

/* Copies as much of str as fits within max_width at glyph_height into
 * out (always NUL-terminated), dropping whole trailing characters with
 * no ellipsis. */
void font_truncate_to_width(const Font *font, const char *str, char *out,
                             size_t out_size, int max_width, int glyph_height);

/* Draws str horizontally centered in [x, x+w) at a fixed glyph_height.
 * unlike font_draw_text_centered, this never shrinks. If str doesn't fit
 * within w at that size, it's truncated and the ellipsis glyph
 * is drawn in place of the dropped tail, so the
 * result always fits. Used for the now-playing title. */
int font_draw_title(const Font *font, SDL_Renderer *renderer, const char *str,
                     int x, int w, int y, int glyph_height);

#endif /* FONT_H */