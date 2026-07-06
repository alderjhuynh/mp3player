#ifndef ID3_H
#define ID3_H

#define ID3_TITLE_MAX 256

/*
 * Fills out_title (best-effort) with a human-readable track title for the
 * mp3 at path, in this priority order:
 *   1. ID3v2 TIT2 frame, if present
 *   2. ID3v1 title field, if present
 *   3. the filename itself, with the extension stripped and
 *      underscores/hyphens turned into spaces
 *
 */
void id3_get_title(const char *path, char out_title[ID3_TITLE_MAX]);

#endif /* ID3_H */
