#include "id3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

static uint32_t read_synchsafe_u32(const unsigned char b[4]) {
    return ((uint32_t)(b[0] & 0x7F) << 21) |
           ((uint32_t)(b[1] & 0x7F) << 14) |
           ((uint32_t)(b[2] & 0x7F) << 7)  |
           ((uint32_t)(b[3] & 0x7F));
}

static uint32_t read_u32(const unsigned char b[4]) {
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8)  | (uint32_t)b[3];
}

static uint32_t read_u24(const unsigned char b[3]) {
    return ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | (uint32_t)b[2];
}

/* Copies a text frame's payload into out (best-effort). frame_data points at
 * the encoding byte; frame_len is the remaining frame size including that byte */
static void extract_text_frame(const unsigned char *frame_data, uint32_t frame_len,
                                char out_title[ID3_TITLE_MAX]) {
    if (frame_len == 0) { out_title[0] = '\0'; return; }

    unsigned char encoding = frame_data[0];
    const unsigned char *text = frame_data + 1;
    uint32_t text_len = frame_len - 1;

    size_t out_i = 0;

    if (encoding == 1 || encoding == 2) {
        /* UTF-16 (LE with BOM, or BE without) */
        uint32_t i = 0;
        bool little_endian = true;
        if (text_len >= 2 && text[0] == 0xFF && text[1] == 0xFE) { i = 2; little_endian = true; }
        else if (text_len >= 2 && text[0] == 0xFE && text[1] == 0xFF) { i = 2; little_endian = false; }

        for (; i + 1 < text_len && out_i < ID3_TITLE_MAX - 1; i += 2) {
            unsigned char lo = little_endian ? text[i] : text[i + 1];
            unsigned char hi = little_endian ? text[i + 1] : text[i];
            if (hi != 0) continue; /* outside ASCII range, drop it */
            if (lo == 0) break;
            out_title[out_i++] = (char)lo;
        }
    } else {
        /* Latin-1 (0) or UTF-8 (3) */
        for (uint32_t i = 0; i < text_len && out_i < ID3_TITLE_MAX - 1; i++) {
            if (text[i] == 0) break;
            out_title[out_i++] = (char)text[i];
        }
    }

    out_title[out_i] = '\0';
}

static bool try_id3v2(FILE *f, char out_title[ID3_TITLE_MAX]) {
    unsigned char header[10];
    if (fread(header, 1, 10, f) != 10) return false;
    if (memcmp(header, "ID3", 3) != 0) return false;

    unsigned char major = header[3];
    uint32_t tag_size = read_synchsafe_u32(&header[6]);

    unsigned char *tag_data = (unsigned char *)malloc(tag_size);
    if (!tag_data) return false;
    if (fread(tag_data, 1, tag_size, f) != tag_size) { free(tag_data); return false; }

    uint32_t pos = 0;
    bool found = false;

    if (major == 2) {
        while (pos + 6 <= tag_size) {
            char id[4] = {(char)tag_data[pos], (char)tag_data[pos+1], (char)tag_data[pos+2], '\0'};
            uint32_t fsize = read_u24(&tag_data[pos + 3]);
            pos += 6;
            if (fsize == 0 || pos + fsize > tag_size) break;
            if (strcmp(id, "TT2") == 0) {
                extract_text_frame(&tag_data[pos], fsize, out_title);
                found = true;
                break;
            }
            pos += fsize;
        }
    } else {
        while (pos + 10 <= tag_size) {
            char id[5] = {(char)tag_data[pos], (char)tag_data[pos+1],
                          (char)tag_data[pos+2], (char)tag_data[pos+3], '\0'};
            uint32_t fsize = (major >= 4) ? read_synchsafe_u32(&tag_data[pos + 4])
                                           : read_u32(&tag_data[pos + 4]);
            pos += 10;
            if (fsize == 0 || pos + fsize > tag_size) break;
            if (strcmp(id, "TIT2") == 0) {
                extract_text_frame(&tag_data[pos], fsize, out_title);
                found = true;
                break;
            }
            pos += fsize;
        }
    }

    free(tag_data);
    return found && out_title[0] != '\0';
}

static bool try_id3v1(FILE *f, char out_title[ID3_TITLE_MAX]) {
    if (fseek(f, -128, SEEK_END) != 0) return false;
    unsigned char tag[128];
    if (fread(tag, 1, 128, f) != 128) return false;
    if (memcmp(tag, "TAG", 3) != 0) return false;

    size_t len = 30;
    while (len > 0 && (tag[3 + len - 1] == ' ' || tag[3 + len - 1] == '\0')) len--;
    if (len == 0) return false;

    size_t copy_len = len < ID3_TITLE_MAX - 1 ? len : ID3_TITLE_MAX - 1;
    memcpy(out_title, &tag[3], copy_len);
    out_title[copy_len] = '\0';
    return true;
}

static void title_from_filename(const char *path, char out_title[ID3_TITLE_MAX]) {
    const char *slash = strrchr(path, '/');
    const char *base = slash ? slash + 1 : path;

    size_t i = 0;
    for (; base[i] != '\0' && base[i] != '.' && i < ID3_TITLE_MAX - 1; i++) {
        char c = base[i];
        out_title[i] = (c == '_' || c == '-') ? ' ' : c;
    }
    out_title[i] = '\0';
}

void id3_get_title(const char *path, char out_title[ID3_TITLE_MAX]) {
    out_title[0] = '\0';

    FILE *f = fopen(path, "rb");
    if (f) {
        if (try_id3v2(f, out_title) && out_title[0] != '\0') {
            fclose(f);
            return;
        }
        rewind(f);
        if (try_id3v1(f, out_title) && out_title[0] != '\0') {
            fclose(f);
            return;
        }
        fclose(f);
    }

    title_from_filename(path, out_title);
}
