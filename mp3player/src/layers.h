#ifndef LAYERS_H
#define LAYERS_H

#include <SDL.h>
#include "config.h"

typedef struct {
    const char *name;
    SDL_Texture *texture;
} Layer;

typedef struct {
    Layer layers[LAYER_COUNT];
    int count;
} LayerStack;

/* Loads every PNG in LAYER_ORDER from assets_dir, in order. Returns 0 on
 * success, -1 on failure  */
int layers_load(LayerStack *stack, SDL_Renderer *renderer, const char *assets_dir);

/* Draws every loaded layer at (0,0), bottom to top. */
void layers_draw(const LayerStack *stack, SDL_Renderer *renderer);

/* Convenience: fetch a texture by filename (for the progress ball) */
SDL_Texture *layers_find(const LayerStack *stack, const char *name);

void layers_destroy(LayerStack *stack);

#endif /* LAYERS_H */
