#ifndef COCOA_SHIM_H
#define COCOA_SHIM_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SDL2 doesn't expose a portable way to make a window borderless AND
 * see-through (only opaque backing stores), so this reaches past it into
 * the underlying NSWindow/Cocoa layer directly.
 *
 * Strips the titlebar, (and with it the miniaturize/close/zoom "traffic
 * light" buttons) and makes the window's background (and the layer SDL's
 * renderer draws into) transparent, so only the opaque pixels the
 * compositor actually paints (the device's PNG layers) are visible on
 * screen; everything else shows the desktop through.
 *
 * macOS/Cocoa only, better operating systems don't need it </3
 *
 * Under Homebrew's current "sdl2" package (which is actually sdl2-compat forwarding
 * onto a real SDL3 runtime), the CAMetalLayer SDL's accelerated renderer
 * draws into isn't attached to the view hierarchy immediately. It only
 * shows up after the window has actually gone through a real display
 * cycle. That means a single call right after SDL_CreateRenderer can find
 * zero CAMetalLayers even though one will exist a few frames later.
 *
 * This function is therefore safe (and cheap) to call repeatedly, like
 * once per frame from the main loop: everything it does (opaque flags,
 * background color, style mask) is idempotent, and it returns the number
 * of CAMetalLayers it found and cleared this call so the caller can stop
 * retrying once that's > 0 (see main.c).
 */
int cocoa_shim_make_borderless_transparent(SDL_Window *window);

#ifdef __cplusplus
}
#endif

#endif /* COCOA_SHIM_H */