#include "playlist.h"
#include "id3.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strcasecmp */

static int has_mp3_ext(const char *name) {
    size_t len = strlen(name);
    if (len < 4) return 0;
    return strcasecmp(name + len - 4, ".mp3") == 0;
}

static int name_cmp(const void *a, const void *b) {
    return strcasecmp(((const Track *)a)->title, ((const Track *)b)->title);
}

int playlist_load(Playlist *pl, const char *dir) {
    memset(pl, 0, sizeof(*pl));

    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "Could not open mp3 library '%s' (no tracks will be "
                        "listed): %s\n", dir, strerror(errno));
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL && pl->count < PLAYLIST_MAX_TRACKS) {
        if (!has_mp3_ext(entry->d_name)) continue;

        Track *t = &pl->tracks[pl->count];
        snprintf(t->path, sizeof(t->path), "%s/%s", dir, entry->d_name);
        /* Prefers an ID3 title if the file has one; otherwise id3_get_title
         * itself falls back to a cleaned-up filename ("song_name.mp3" ->
         * "song name", this fails if your title has a period in it, so
         * ill attempt to fix this for the next stable distro). */
        id3_get_title(t->path, t->title);
        pl->count++;
    }
    closedir(d);

    qsort(pl->tracks, (size_t)pl->count, sizeof(Track), name_cmp);
    return pl->count;
}