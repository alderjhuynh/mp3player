#define MINIAUDIO_IMPLEMENTATION
#include "player.h"
#include <string.h>
#include <stdio.h>

void player_init(PlayerState *p) {
    memset(p, 0, sizeof(*p));

    ma_result result = ma_engine_init(NULL, &p->engine);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "player: failed to init audio engine (%d) -- "
                        "playback will be unavailable.\n", (int)result);
        p->engine_ready = false;
        return;
    }
    p->engine_ready = true;
}

bool player_load_track(PlayerState *p, const char *path, const char *title) {
    if (!p->engine_ready) return false;

    if (p->sound_loaded) {
        ma_sound_uninit(&p->sound);
        p->sound_loaded = false;
    }

    ma_result result = ma_sound_init_from_file(&p->engine, path, 0, NULL, NULL, &p->sound);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "player: failed to load '%s' (%d)\n", path, (int)result);
        p->has_track = false;
        return false;
    }
    p->sound_loaded = true;

    float length_seconds = 0.0f;
    ma_sound_get_length_in_seconds(&p->sound, &length_seconds);

    strncpy(p->title, title, PLAYER_TITLE_MAX - 1);
    p->title[PLAYER_TITLE_MAX - 1] = '\0';
    p->duration_seconds = (double)length_seconds;
    p->elapsed_seconds = 0.0;
    p->is_playing = false;
    p->has_track = true;
    return true;
}

void player_play(PlayerState *p) {
    if (!p->has_track || !p->sound_loaded) return;
    ma_sound_start(&p->sound);
    p->is_playing = true;
}

void player_toggle_play_pause(PlayerState *p) {
    if (!p->has_track || !p->sound_loaded) return;
    if (p->is_playing) {
        ma_sound_stop(&p->sound);
        p->is_playing = false;
    } else {
        ma_sound_start(&p->sound);
        p->is_playing = true;
    }
}

static void seek_to_seconds(PlayerState *p, double seconds) {
    if (seconds < 0.0) seconds = 0.0;
    if (seconds > p->duration_seconds) seconds = p->duration_seconds;
    ma_sound_seek_to_second(&p->sound, (float)seconds);
    p->elapsed_seconds = seconds; /* instant UI feedback; player_tick() will
                                     confirm it from the real cursor next frame */
}

void player_seek_relative(PlayerState *p, double delta_seconds) {
    if (!p->has_track || !p->sound_loaded) return;
    seek_to_seconds(p, p->elapsed_seconds + delta_seconds);
}

void player_seek_to_fraction(PlayerState *p, double fraction) {
    if (!p->has_track || !p->sound_loaded) return;
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    seek_to_seconds(p, fraction * p->duration_seconds);
}

void player_tick(PlayerState *p, double dt_seconds) {
    (void)dt_seconds;
    if (!p->has_track || !p->sound_loaded) return;

    float cursor_seconds = 0.0f;
    if (ma_sound_get_cursor_in_seconds(&p->sound, &cursor_seconds) == MA_SUCCESS) {
        p->elapsed_seconds = (double)cursor_seconds;
    }

    if (p->is_playing && ma_sound_at_end(&p->sound)) {
        p->is_playing = false;
        p->elapsed_seconds = p->duration_seconds;
    }
}

double player_progress_fraction(const PlayerState *p) {
    if (!p->has_track || p->duration_seconds <= 0.0) return 0.0;
    double f = p->elapsed_seconds / p->duration_seconds;
    if (f < 0.0) f = 0.0;
    if (f > 1.0) f = 1.0;
    return f;
}

void player_play_sfx(PlayerState *p, const char *path) {
    if (!p->engine_ready) return;
    /* ma_engine_play_sound() decodes/starts path as a detached,
     * fire-and-forget sound on the engine's own internal sound group and
     * frees it automatically once it finishes no ma_sound to hold onto
     * or uninit ourselves, and it layers on top of whatever p->sound (the
     * actual track) is doing without interrupting it. */
    ma_result result = ma_engine_play_sound(&p->engine, path, NULL);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "player: failed to play sfx '%s' (%d)\n", path, (int)result);
    }
}

void player_shutdown(PlayerState *p) {
    if (p->sound_loaded) {
        ma_sound_uninit(&p->sound);
        p->sound_loaded = false;
    }
    if (p->engine_ready) {
        ma_engine_uninit(&p->engine);
        p->engine_ready = false;
    }
}
