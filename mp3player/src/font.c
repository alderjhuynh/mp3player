#include "font.h"
#include "config.h"
#include <SDL_image.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

int font_load(Font *font, SDL_Renderer *renderer, const char *fonts_dir) {
    memset(font, 0, sizeof(*font));

    char path[1024];

    snprintf(path, sizeof(path), "%s/number_atlas.png", fonts_dir);
    font->digits_tex = IMG_LoadTexture(renderer, path);
    if (!font->digits_tex) {
        fprintf(stderr, "Failed to load font atlas '%s': %s\n", path, IMG_GetError());
        return -1;
    }
    SDL_SetTextureBlendMode(font->digits_tex, SDL_BLENDMODE_BLEND);

    snprintf(path, sizeof(path), "%s/text_atlas.png", fonts_dir);
    font->letters_tex = IMG_LoadTexture(renderer, path);
    if (!font->letters_tex) {
        fprintf(stderr, "Failed to load font atlas '%s': %s\n", path, IMG_GetError());
        SDL_DestroyTexture(font->digits_tex);
        font->digits_tex = NULL;
        return -1;
    }
    SDL_SetTextureBlendMode(font->letters_tex, SDL_BLENDMODE_BLEND);

    snprintf(path, sizeof(path), "%s/elipses.png", fonts_dir);
    font->ellipsis_tex = IMG_LoadTexture(renderer, path);
    if (!font->ellipsis_tex) {
        fprintf(stderr, "Failed to load font atlas '%s': %s\n", path, IMG_GetError());
        SDL_DestroyTexture(font->digits_tex);
        SDL_DestroyTexture(font->letters_tex);
        font->digits_tex = NULL;
        font->letters_tex = NULL;
        return -1;
    }
    SDL_SetTextureBlendMode(font->ellipsis_tex, SDL_BLENDMODE_BLEND);

    return 0;
}

void font_destroy(Font *font) {
    if (font->digits_tex) SDL_DestroyTexture(font->digits_tex);
    if (font->letters_tex) SDL_DestroyTexture(font->letters_tex);
    if (font->ellipsis_tex) SDL_DestroyTexture(font->ellipsis_tex);
    font->digits_tex = NULL;
    font->letters_tex = NULL;
    font->ellipsis_tex = NULL;
}

static int digit_glyph_index(char c) {
    const char *order = FONT_DIGIT_GLYPH_ORDER;
    for (int i = 0; order[i] != '\0'; i++) {
        if (order[i] == c) return i;
    }
    return -1;
}

/* A glyph's advance width in pixels at a given rendering scale */
static int glyph_advance(char c, double scale) {
    if (digit_glyph_index(c) >= 0) {
        return (int)SDL_round(FONT_DIGIT_CELL_W * scale);
    }
    return (int)SDL_round(FONT_LETTER_CELL_W * scale);
}

static int glyph_spacing(double scale) {
    int s = (int)SDL_round(3.0 * scale);
    return s < 1 ? 1 : s;
}

int font_text_width(const Font *font, const char *str, int glyph_height) {
    (void)font;
    if (!str || !*str) return 0;
    double scale = (double)glyph_height / (double)FONT_LETTER_CELL_H;
    int spacing = glyph_spacing(scale);
    int width = 0;
    for (const char *p = str; *p; p++) {
        width += glyph_advance(*p, scale);
        if (*(p + 1)) width += spacing;
    }
    return width;
}

int font_draw_text(const Font *font, SDL_Renderer *renderer, const char *str,
                    int x, int y, int glyph_height) {
    if (!str || !*str) return 0;
    double scale = (double)glyph_height / (double)FONT_LETTER_CELL_H;
    int spacing = glyph_spacing(scale);

    int cursor = x;
    for (const char *p = str; *p; p++) {
        char c = (char)toupper((unsigned char)*p);

        int digit_idx = digit_glyph_index(c);
        if (digit_idx >= 0) {
            SDL_Rect src = {FONT_DIGIT_MARGIN_X + digit_idx * FONT_DIGIT_CELL_W, 0,
                             FONT_DIGIT_CELL_W, FONT_DIGIT_CELL_H};
            int dw = (int)SDL_round(FONT_DIGIT_CELL_W * scale);
            int dh = (int)SDL_round(FONT_DIGIT_CELL_H * scale);
            SDL_Rect dst = {cursor, y + (glyph_height - dh), dw, dh};
            if (renderer && font->digits_tex) {
                SDL_RenderCopy(renderer, font->digits_tex, &src, &dst);
            }
            cursor += dw;
        } else if (c >= 'A' && c <= 'Z') {
            int idx = c - 'A';
            SDL_Rect src = {idx * FONT_LETTER_CELL_W, 0, FONT_LETTER_CELL_W, FONT_LETTER_CELL_H};
            int dw = (int)SDL_round(FONT_LETTER_CELL_W * scale);
            SDL_Rect dst = {cursor, y, dw, glyph_height};
            if (renderer && font->letters_tex) {
                SDL_RenderCopy(renderer, font->letters_tex, &src, &dst);
            }
            cursor += dw;
        } else {
            /* No glyph for this character (space, punctuation, etc.)
             * just leave a blank gap the width of a letter. this lowkey sucks ass im ngl */
            cursor += (int)SDL_round(FONT_LETTER_CELL_W * scale);
        }

        if (*(p + 1)) cursor += spacing;
    }

    return cursor - x;
}

int font_draw_text_centered(const Font *font, SDL_Renderer *renderer,
                             const char *str, int x, int w, int y,
                             int glyph_height) {
    int text_w = font_text_width(font, str, glyph_height);
    int draw_x = x + (w - text_w) / 2;
    if (draw_x < x) draw_x = x;
    return font_draw_text(font, renderer, str, draw_x, y, glyph_height);
}

int font_fit_height(const Font *font, const char *str, int max_width,
                     int min_height, int max_height) {
    int h = max_height;
    while (h > min_height && font_text_width(font, str, h) > max_width) {
        h -= 1;
    }
    return h;
}

void font_truncate_to_width(const Font *font, const char *str, char *out,
                             size_t out_size, int max_width, int glyph_height) {
    (void)font;
    if (out_size == 0) return;
    out[0] = '\0';
    if (!str || !*str) return;

    double scale = (double)glyph_height / (double)FONT_LETTER_CELL_H;
    int spacing = glyph_spacing(scale);

    size_t n = 0;
    int width = 0;
    for (const char *p = str; *p && n + 1 < out_size; p++) {
        int adv = glyph_advance(*p, scale);
        int next_width = width + adv + (n > 0 ? spacing : 0);
        if (next_width > max_width) break;
        width = next_width;
        out[n++] = *p;
    }
    out[n] = '\0';
}

int font_draw_title(const Font *font, SDL_Renderer *renderer, const char *str,
                     int x, int w, int y, int glyph_height) {
    if (!str || !*str) return 0;

    int full_width = font_text_width(font, str, glyph_height);
    if (full_width <= w) {
        return font_draw_text_centered(font, renderer, str, x, w, y, glyph_height);
    }

    double scale = (double)glyph_height / (double)FONT_LETTER_CELL_H;
    int spacing = glyph_spacing(scale);
    int ellipsis_w = (int)SDL_round(FONT_ELLIPSIS_CELL_W * scale);

    /* Reserve room for the ellipsis glyph
     *  before truncating the
     * text itself into what's left. */
    int text_max_w = w - ellipsis_w - spacing;
    if (text_max_w < 0) text_max_w = 0;

    char truncated[256];
    font_truncate_to_width(font, str, truncated, sizeof(truncated), text_max_w, glyph_height);

    int text_w = font_text_width(font, truncated, glyph_height);
    int total_w = text_w + (text_w > 0 ? spacing : 0) + ellipsis_w;

    int draw_x = x + (w - total_w) / 2;
    if (draw_x < x) draw_x = x;

    int drawn = font_draw_text(font, renderer, truncated, draw_x, y, glyph_height);

    int ellipsis_x = draw_x + drawn + (text_w > 0 ? spacing : 0);
    int dh = (int)SDL_round(FONT_ELLIPSIS_CELL_H * scale);
    SDL_Rect src = {0, 0, FONT_ELLIPSIS_CELL_W, FONT_ELLIPSIS_CELL_H};
    SDL_Rect dst = {ellipsis_x, y + (glyph_height - dh), ellipsis_w, dh};
    if (renderer && font->ellipsis_tex) {
        SDL_RenderCopy(renderer, font->ellipsis_tex, &src, &dst);
    }

    return total_w;
}