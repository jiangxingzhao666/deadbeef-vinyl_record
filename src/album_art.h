/**
 * @file album_art.h
 * @brief 专辑封面获取 —— 公共接口
 *
 * 从 deadbeef 曲目中获取专辑封面图像，
 * 优先使用元数据指定的路径，兜底扫描文件所在目录。
 */

#ifndef ALBUM_ART_H
#define ALBUM_ART_H

#include <cairo.h>
#include <deadbeef/deadbeef.h>

cairo_surface_t *album_art_get(DB_functions_t *api, DB_playItem_t *it, int size);

#endif
