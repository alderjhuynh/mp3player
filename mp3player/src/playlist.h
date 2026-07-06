#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "config.h"

typedef struct {
    char title[PLAYLIST_TITLE_MAX]; /* derived from filename, for display */
    char path[1024];                /* full path, for future real loading */
} Track;

typedef struct {
    Track tracks[PLAYLIST_MAX_TRACKS];
    int count;
} Playlist;

/* Scans `dir` for *.mp3 files (case-insensitive extension), sorted
 * alphabetically by filename, up to PLAYLIST_MAX_TRACKS entries. Titles are
 * derived from the filename with the extension stripped and '_'/'-' turned
 * into spaces -- font_draw_text upper-cases at draw time, so case doesn't
 * matter here.
 *
 * Always leaves *pl in a valid (possibly empty, count == 0) state so the
 * library view has something sane to show even if the directory is
 * missing. Returns the number of tracks found, or -1 if the directory
 * couldn't be opened at all. */
int playlist_load(Playlist *pl, const char *dir);

#endif /* PLAYLIST_H */