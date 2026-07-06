#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "../third_party/miniaudio.h"

#define PLAYER_TITLE_MAX 256

typedef struct {
    char title[PLAYER_TITLE_MAX];
    double duration_seconds;
    double elapsed_seconds; /* refreshed from the real playback clock every player_tick() */
    bool is_playing;
    bool has_track;

    ma_engine engine;
    bool engine_ready;

    ma_sound sound;
    bool sound_loaded;
} PlayerState;

/* Starts the miniaudio engine. Playback stays silent/inert until player_load_track() is called. */
void player_init(PlayerState *p);

/* Stops/unloads whatever was playing, loads `path`, and primes it paused at
 * 0:00. title is what the now-playing screen displays (see id3.c /
 * playlist.c for where that string comes from). Does not start playback. */
bool player_load_track(PlayerState *p, const char *path, const char *title);

void player_play(PlayerState *p);
void player_toggle_play_pause(PlayerState *p);

/* Reverse/forward buttons: nudge the playhead by delta_seconds (negative to
 * rewind), clamped to [0, duration]. Actually seeks the audio, not just the
 * displayed number. */
void player_seek_relative(PlayerState *p, double delta_seconds);

/* Progress-ball scrubbing: jump straight to a 0.0-1.0 point in the track. */
void player_seek_to_fraction(PlayerState *p, double fraction);

/* Call once per frame: refreshes elapsed_seconds from the real playback
 * cursor and notices when a track has finished. */
void player_tick(PlayerState *p, double dt_seconds);

double player_progress_fraction(const PlayerState *p);

/* unused */
void player_play_sfx(PlayerState *p, const char *path);

void player_shutdown(PlayerState *p);

#endif /* PLAYER_H */
