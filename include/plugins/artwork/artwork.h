/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __ARTWORK_H
#define __ARTWORK_H

#include <stdint.h>
#include <deadbeef/deadbeef.h>

#define DDB_ARTWORK_MAJOR_VERSION 2
#define DDB_ARTWORK_MINOR_VERSION 0

enum {
    DDB_ARTWORK_FLAG_CANCELLED = (1<<0),
};

typedef struct ddb_cover_query_s {
    uint32_t _size;
    void *user_data;
    uint32_t flags;
    ddb_playItem_t *track;
    int64_t source_id;
} ddb_cover_query_t;

typedef struct ddb_cover_info_s {
    size_t _size;
    void *priv;
    int cover_found;
    char *image_filename;
} ddb_cover_info_t;

typedef void (*ddb_cover_callback_t) (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover);

typedef enum {
    DDB_ARTWORK_SETTINGS_DID_CHANGE = 1,
} ddb_artwork_listener_event_t;

typedef void (*ddb_artwork_listener_t) (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2);

typedef struct {
    DB_misc_t plugin;

    void (*cover_get) (ddb_cover_query_t *query, ddb_cover_callback_t callback);
    void (*reset) (void);
    void (*cover_info_release) (ddb_cover_info_t *cover);
    void (*add_listener) (ddb_artwork_listener_t listener, void *user_data);
    void (*remove_listener) (ddb_artwork_listener_t listener, void *user_data);
    void (*default_image_path) (char *path, size_t size);
    int64_t (*allocate_source_id) (void);
    void (*cancel_queries_with_source_id) (int64_t source_id);
} ddb_artwork_plugin_t;

#endif /*__ARTWORK_H*/
