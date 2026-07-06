#include "layers.h"
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>

int layers_load(LayerStack *stack, SDL_Renderer *renderer, const char *assets_dir) {
    stack->count = 0;

    for (size_t i = 0; i < LAYER_COUNT; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", assets_dir, LAYER_ORDER[i]);

        SDL_Texture *tex = IMG_LoadTexture(renderer, path);
        if (!tex) {
            fprintf(stderr, "Failed to load layer '%s': %s\n", path, IMG_GetError());
            return -1;
        }

        stack->layers[stack->count].name = LAYER_ORDER[i];
        stack->layers[stack->count].texture = tex;
        stack->count++;
    }

    return 0;
}

SDL_Texture *layers_find(const LayerStack *stack, const char *name) {
    for (int i = 0; i < stack->count; i++) {
        if (strcmp(stack->layers[i].name, name) == 0) {
            return stack->layers[i].texture;
        }
    }
    return NULL;
}

void layers_draw(const LayerStack *stack, SDL_Renderer *renderer) {
    SDL_Rect dst = {0, 0, CANVAS_W, CANVAS_H};

    for (int i = 0; i < stack->count; i++) {
        /* progress_ball.png is repositioned every frame by ui.c based on
         * playback progress, so it is drawn separately (still on top,
         * since it's last in LAYER_ORDER) instead of pinned at (0,0). */
        if (strcmp(stack->layers[i].name, "progress_ball.png") == 0) {
            continue;
        }
        SDL_RenderCopy(renderer, stack->layers[i].texture, NULL, &dst);
    }
}

void layers_destroy(LayerStack *stack) {
    for (int i = 0; i < stack->count; i++) {
        if (stack->layers[i].texture) {
            SDL_DestroyTexture(stack->layers[i].texture);
            stack->layers[i].texture = NULL;
        }
    }
    stack->count = 0;
}
