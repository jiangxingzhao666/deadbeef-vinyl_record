/**
 * @file album_art.c
 * @brief 专辑封面获取 —— 实现
 *
 * 查找策略（按优先级）：
 *   0. 直接提取 ID3v2 APIC 内嵌封面（无条件，所有本地文件）
 *   1. "album_art" 元数据键（deadbeef artwork 插件缓存）
 *   2. ":ALBUM_ART_FILE" 元数据键（备用）
 *   3. 曲目文件所在目录扫描常见封面文件名
 *
 * 平台兼容性：
 *   - 所有文件操作使用 GLib API（g_file_test, GdkPixbuf）
 *   - Windows 上 GLib 内部转为宽字符 API，原生支持 UTF-8 路径
 *   - 避免 CRT 的 stat()/fopen() 无法处理 CJK 路径的问题
 */

#include "album_art.h"
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif


/* ================================================================
 *  封面文件名候选列表
 * ================================================================ */

static const char *cover_names[] = {
    "cover.jpg",       "cover.png",
    "folder.jpg",      "folder.png",
    "front.jpg",       "front.png",
    "album.jpg",       "album.png",
    "AlbumArt.jpg",    "AlbumArtSmall.jpg",
    ".folder.png",
    "Cover.jpg",       "Cover.png",
    "Folder.jpg",      "Folder.png",
    "Front.jpg",       "Front.png",
    NULL
};


/* ================================================================
 *  文件系统工具
 * ================================================================ */

static int
file_exists(const char *path)
{
    return g_file_test(path, G_FILE_TEST_IS_REGULAR) ? 1 : 0;
}


static char *
get_dir(const char *filepath)
{
    char *p = strdup(filepath);

    char *slash = strrchr(p, '/');
#ifdef _WIN32
    char *bslash = strrchr(p, '\\');
    if (bslash > slash) {
        slash = bslash;
    }
#endif
    if (slash) {
        *slash = '\0';
    } else {
        p[0] = '.';
        p[1] = '\0';
    }
    return p;
}


/* ================================================================
 *  图像加载
 * ================================================================ */

static cairo_surface_t *
load_image(const char *path, int size)
{
    if (!path || !path[0]) {
        return NULL;
    }

    GError *err = NULL;
    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(
        path, size, size, TRUE, &err);
    if (pb) {
        cairo_surface_t *surf = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            gdk_pixbuf_get_width(pb),
            gdk_pixbuf_get_height(pb));
        cairo_t *cr = cairo_create(surf);
        gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        g_object_unref(pb);
        return surf;
    }
    if (err) {
        g_error_free(err);
    }

    cairo_surface_t *img = cairo_image_surface_create_from_png(path);
    if (cairo_surface_status(img) == CAIRO_STATUS_SUCCESS) {
        return img;
    }

    cairo_surface_destroy(img);
    return NULL;
}


static int
scan_dir_for_cover(const char *dir, char *out, int out_size)
{
    for (int i = 0; cover_names[i]; i++) {
        snprintf(out, out_size, "%s/%s", dir, cover_names[i]);
        if (file_exists(out)) {
            return 1;
        }
    }
    return 0;
}


static const char *
strip_file_prefix(const char *uri)
{
    if (strncmp(uri, "file://", 7) == 0) {
        const char *p = uri + 7;
#ifdef _WIN32
        if (*p == '/') {
            p++;
        }
#endif
        return p;
    }
    return uri;
}


/* ================================================================
 *  公开 API
 * ================================================================ */

cairo_surface_t *
album_art_get(DB_functions_t *api, DB_playItem_t *it, int size)
{
    if (!it) {
        return NULL;
    }

    char art_path[2048] = {0};
    int n;

    char uri[2048] = {0};
    api->pl_get_meta(it, ":URI", uri, sizeof(uri));
    const char *fpath = strip_file_prefix(uri);


    /* ---- 方法0: 直接提取 ID3v2 APIC 内嵌封面 ---- */
    if (fpath && fpath[0] && file_exists(fpath)) {
        DB_FILE *fp = api->fopen(fpath);
        if (fp) {
            DB_id3v2_tag_t tag;
            memset(&tag, 0, sizeof(tag));
            int result = api->junk_id3v2_read_full(NULL, &tag, fp);
            api->fclose(fp);

            if (result == 0) {
                DB_id3v2_frame_t *frame;
                for (frame = tag.frames; frame; frame = frame->next) {
                    if (memcmp(frame->id, "APIC", 4) != 0 || frame->size == 0) {
                        continue;
                    }

                    const uint8_t *data = frame->data;
                    uint32_t remaining = frame->size;

                    if (remaining < 1) { continue; }
                    data++; remaining--;

                    while (remaining > 0 && *data != 0) {
                        data++; remaining--;
                    }
                    if (remaining < 1) { continue; }
                    data++; remaining--;

                    if (remaining < 1) { continue; }
                    data++; remaining--;

                    while (remaining > 0 && *data != 0) {
                        data++; remaining--;
                    }
                    if (remaining < 1) { continue; }
                    data++; remaining--;

                    if (remaining > 0) {
                        GInputStream *stream =
                            g_memory_input_stream_new_from_data(
                                data, remaining, NULL);
                        GdkPixbuf *pb = gdk_pixbuf_new_from_stream_at_scale(
                            stream, size, size, TRUE, NULL, NULL);
                        g_object_unref(stream);

                        if (pb) {
                            cairo_surface_t *surf = cairo_image_surface_create(
                                CAIRO_FORMAT_ARGB32,
                                gdk_pixbuf_get_width(pb),
                                gdk_pixbuf_get_height(pb));
                            cairo_t *cr = cairo_create(surf);
                            gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);
                            cairo_paint(cr);
                            cairo_destroy(cr);
                            g_object_unref(pb);
                            api->junk_id3v2_free(&tag);
                            return surf;
                        }
                    }
                    break;
                }
                api->junk_id3v2_free(&tag);
            }
        }
    }


    /* ---- 方法1: album_art 元数据（artwork 插件提供） ---- */
    n = api->pl_get_meta(it, "album_art", art_path, sizeof(art_path));
    if (n > 0 && file_exists(art_path)) {
        return load_image(art_path, size);
    }

    /* ---- 方法2: :ALBUM_ART_FILE 元数据（备用） ---- */
    n = api->pl_get_meta(it, ":ALBUM_ART_FILE", art_path, sizeof(art_path));
    if (n > 0 && file_exists(art_path)) {
        return load_image(art_path, size);
    }

    /* ---- 方法3: 目录扫描（含父目录） ---- */
    if (fpath && fpath[0]) {
        char *dir = get_dir(fpath);
        int found = scan_dir_for_cover(dir, art_path, sizeof(art_path));

        if (!found) {
            char *parent = get_dir(dir);
            if (parent && strcmp(parent, dir) != 0) {
                found = scan_dir_for_cover(parent, art_path, sizeof(art_path));
            }
            free(parent);
        }
        free(dir);

        if (found) {
            return load_image(art_path, size);
        }
    }

    return NULL;
}
