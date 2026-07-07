# mp3player

A pixel-art, click-wheel mp3 player for the desktop, written in C with SDL2 and [miniaudio](https://miniaudio.dev/). The whole UI is a stack of pre-positioned PNG layers rendered on a fixed 500x900 canvas. No widget toolkit, no window chrome, just a borderless "device" you can drag around your screen and click on.

## Requirements

- `clang` (also works as the Objective-C compiler on macOS)
- `make`
- `pkg-config`
- SDL2 and SDL2_image development libraries

On macOS, `make app` additionally uses `sips`, `iconutil`, `xattr`, and `codesign` (all part of the standard Xcode command-line tools) to produce a proper `.app` bundle with an icon.

Install SDL on macOS via Homebrew:

```bash
brew install sdl2 sdl2_image pkg-config
```

On Linux (Debian/Ubuntu):

```bash
sudo apt install clang make pkg-config libsdl2-dev libsdl2-image-dev
```

## Build & Run

| Command | What it does |
|---|---|
| `make` / `make all` | Compiles the project and produces the `mp3player` binary in the repo root |
| `make run` | Builds (if needed) and immediately runs `./mp3player` |
| `make clean` | Removes object files, the binary, and the `.app` bundle |
| `make app` | **macOS only.** Builds the binary and packages it into `MP3Player.app` (bundles `assets/`, writes an `Info.plist`, generates an `.icns` from `assets/reference/composited.png`, and ad-hoc code-signs the bundle) |
| `make app-run` | Builds the `.app` (if needed) and opens it with `open` |
| `make app-icon` | Regenerates just the `.icns` icon from the reference PNG |
| `make app-clean` | Removes the `.app` bundle only |

`make app` is guarded to only run on macOS — it packages a Cocoa window, CoreAudio, and AudioToolbox, so building it on Linux would just zip up a non-functional binary.

On Linux, miniaudio's threading/dynamic-loading backend pulls in `-lpthread -ldl`. On macOS, the Makefile instead links `Cocoa`, `QuartzCore`, `CoreAudio`, `AudioToolbox`, and `CoreFoundation`, and compiles `src/cocoa_shim.m` as the project's one Objective-C file.

Run the binary from the project root (or via `make run`/`make app-run`) so the relative `assets/` paths resolve correctly — `main.c` also `chdir`s to the executable's own directory on macOS so bundled `.app` runs work regardless of your current working directory.

## Adding music

Drop `.mp3` files into:

```
assets/audio/mp3/
```

`playlist_load()` scans that directory for anything ending in `.mp3` (case-insensitive), sorts alphabetically, and lists up to `PLAYLIST_MAX_TRACKS` (64) tracks. Track titles come from `id3.c`, in priority order: ID3v2 `TIT2` frame → ID3v1 title field → the filename itself (underscores/hyphens turned into spaces).

## Controls

The whole interface is one click wheel plus a progress bar, matching a classic clickwheel mp3 player:

- **Top (menu):** toggle between the *Now Playing* screen and the *Library* screen
- **Left / Right (reverse / forward):**
  - Now Playing: seek backward/forward
  - Library: move the selection up/down
- **Bottom (play/pause):**
  - Now Playing: toggle playback
  - Library: load and play the highlighted track, then switch back to Now Playing
- **Progress ball:** click or drag along the track to scrub
- **Anywhere on the device body:** click and drag to move the (borderless, titlebar-less) window

## Code layout

```
src/
├── main.c           # SDL setup, window/renderer creation, the event + render loop, HiDPI mouse-coordinate handling
├── config.h          # All layout/geometry constants: canvas size, layer draw order, click-wheel geometry, font atlas layout, asset paths
├── layers.c/h        # Loads and draws the stacked PNG layers that make up the device skin
├── player.c/h        # Wraps miniaudio: load/play/pause/seek a track, tick the playback clock, play one-shot UI sound effects
├── ui.c/h             # Click-wheel hit-testing (which quadrant was clicked) and progress-ball drawing/geometry
├── screen.c/h         # The two app "screens" (Now Playing / Library): routes wheel input per-screen and draws their text/rows
├── playlist.c/h       # Scans the mp3 directory into a Playlist of Track entries
├── font.c/h           # Bitmap-font rendering from the digit/letter/ellipsis texture atlases (digits + uppercase A-Z only)
├── id3.c/h            # Minimal ID3v2/ID3v1 tag reader for track titles
└── cocoa_shim.m/h     # macOS-only: reaches into Cocoa to strip the titlebar and make the window background transparent, since SDL2 has no portable API for a see-through borderless window

third_party/
└── miniaudio.h       # Single-header audio playback library (github.com/mackron/miniaudio)
```

### A few implementation notes (from the code itself)

- **Rendering model:** every PNG in `assets/visual/` is pre-exported at its final position on the same 500x900 canvas, so drawing is just "blit every layer at `(0,0)`, bottom to top" — no per-layer offsets needed (see `LAYER_ORDER` in `config.h`).
- **HiDPI mouse handling:** `main.c` compensates for a quirk where Homebrew's `sdl2` package is actually `sdl2-compat` forwarding onto SDL3, which, unlike classic SDL2, delivers mouse coordinates already in physical-pixel space on Retina displays. `translate_mouse_to_logical()` detects the mismatch and skips the usual point-to-logical scaling in that case.
- **Window transparency:** SDL2 can make a window borderless, but not both borderless *and* see-through. `cocoa_shim.m` works around this by talking to the underlying `NSWindow` directly, and is retried once per frame at startup until it successfully finds the Metal layer to make transparent (see the `cocoa_shim_pending` loop in `main.c`).
- **Fonts:** the bitmap font atlases only contain digits and uppercase A-Z; `font_draw_text()` upper-cases input and treats anything without a glyph (spaces, punctuation) as blank advance — which is also why `"12:34 / 3:45"`-style time strings just work, with `/` rendering as a gap.
- **Sound effects:** `SFX_UI_CLICK` and `player_play_sfx()` exist and work (fire-and-forget playback layered over the current track) but aren't currently wired into any interaction.
- **Asset paths are relative,** the code assumes it's run from the project root (or, on macOS, from inside the `.app` bundle after `chdir_to_executable_dir()` runs).

## License

MIT — see [LICENSE](LICENSE).
