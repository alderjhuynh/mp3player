#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <SDL_syswm.h>
#include <stdio.h>
#include "cocoa_shim.h"

/* recurse to arbitrary depth rather than a fixed sublayer/sub-sublayer
 * pair. Under sdl2-compat/SDL3 there's no guarantee the CAMetalLayer
 * sits exactly two levels down from contentView.layer, ;-; */
static void clear_layer_opacity_recursive(CALayer *layer, int *metal_layers_found) {
    if (!layer) return;
    layer.opaque = NO;
    if ([layer isKindOfClass:[CAMetalLayer class]]) {
        ((CAMetalLayer *)layer).opaque = NO;
        (*metal_layers_found)++;
    }
    for (CALayer *sub in layer.sublayers) {
        clear_layer_opacity_recursive(sub, metal_layers_found);
    }
}

int cocoa_shim_make_borderless_transparent(SDL_Window *window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        fprintf(stderr, "cocoa_shim: SDL_GetWindowWMInfo failed: %s\n", SDL_GetError());
        return -1;
    }
    if (wmInfo.subsystem != SDL_SYSWM_COCOA) {
        fprintf(stderr, "cocoa_shim: not running under Cocoa, skipping.\n");
        return -1;
    }

    NSWindow *nsWindow = wmInfo.info.cocoa.window;
    if (!nsWindow) {
        fprintf(stderr, "cocoa_shim: wmInfo.info.cocoa.window was NULL.\n");
        return -1;
    }

    nsWindow.opaque = NO;
    nsWindow.backgroundColor = [NSColor clearColor];
    /* the case art is a rounded shape, not a plain rectangle.
     * AppKit's default drop shadow is a rectangular shadow of the whole
     * window frame, which would show up as a visible box around the
     * transparent parts. Buh bye. */
    nsWindow.hasShadow = NO;
    nsWindow.styleMask = NSWindowStyleMaskBorderless;

    NSView *contentView = nsWindow.contentView;
    if (!contentView) {
        fprintf(stderr, "cocoa_shim: nsWindow.contentView was nil -- nothing to mark transparent.\n");
        return -1;
    }

    contentView.wantsLayer = YES;
    contentView.layer.opaque = NO;
    contentView.layer.backgroundColor = [[NSColor clearColor] CGColor];

    /* SDL's accelerated renderer on macOS draws into a CAMetalLayer that it
     * inserts somewhere in this view's layer tree. That layer defaults to
     * opaque too, which would paint solid black wherever our PNGs are
     * transparent, so walk the whole tree and clear opacity on every 
     * CAMetalLayer we find. */
    int metal_layers_found = 0;
    clear_layer_opacity_recursive(contentView.layer, &metal_layers_found);

    if (metal_layers_found > 0) {
        fprintf(stderr, "cocoa_shim: found and cleared %d CAMetalLayer(s) -- "
                        "transparency should now composite correctly.\n",
                metal_layers_found);
    }
    return metal_layers_found;
}
