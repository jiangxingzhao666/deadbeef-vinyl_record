/**
 * @file MyPlugin.c
 * @brief DeaDBeeF Vinyl Record 插件主文件
 *
 * 功能：将当前播放曲目的专辑封面渲染为黑胶唱片样式，
 * 播放时唱片旋转，暂停时停止旋转，无封面时显示默认黑色唱片。
 * 通过 deadbeef 设计模式嵌入主窗口布局。
 *
 * 封面获取策略（同步，所有操作在主线程）：
 *   1. 直接提取 ID3v2 APIC 内嵌封面（不依赖 artwork 插件）
 *   2. 元数据键 album_art / :ALBUM_ART_FILE（artwork 插件缓存）
 *   3. 目录扫描常见封面文件名
 *
 * 依赖：deadbeef 播放器 API、GTK+ 3、Cairo 图形库
 */

#include <deadbeef/deadbeef.h>
#include <gtk/gtk.h>
#include "vinyl_widget.h"
#include "album_art.h"

/* GTKUI 设计模式控件注册 API */
#include <plugins/gtkui/gtkui_api.h>

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

/* ================================================================
 *  全局状态
 * ================================================================ */

static DB_functions_t *deadbeef;
static ddb_gtkui_t    *gtkui_plugin_api;

static VinylWidget *vinyl     = NULL;
static GtkWidget   *container = NULL;


/* ================================================================
 *  控件数据结构 —— 继承 ddb_gtkui_widget_t
 * ================================================================ */

typedef struct {
    ddb_gtkui_widget_t base;
} vinyl_record_widget_t;


/* ================================================================
 *  封面更新
 * ================================================================ */

/**
 * @brief 更新专辑封面
 *
 * 调用 album_art_get 同步获取封面（元数据键 → ID3v2 提取 → 目录扫描）。
 */
static void
update_album_art(DB_playItem_t *it)
{
    if (!vinyl || !it) {
        return;
    }

    cairo_surface_t *art = album_art_get(deadbeef, it, 256);
    vinyl_widget_set_album_art(vinyl, art);
    if (art) {
        cairo_surface_destroy(art);
    }
}


/* ================================================================
 *  事件消息转发 —— 接收 deadbeef 播放事件并更新控件
 * ================================================================ */

/**
 * @brief 处理 deadbeef 广播消息（播放开始、暂停、停止、切歌）
 */
static int
vinyl_record_message(ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx,
                     uint32_t p1, uint32_t p2)
{
    (void)w;

    switch (id) {

    case DB_EV_SONGSTARTED: {
        if (vinyl) {
            vinyl_widget_set_playing(vinyl, TRUE);

            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            update_album_art(ev->track);
        }
        break;
    }

    case DB_EV_PAUSED: {
        if (vinyl) {
            vinyl_widget_set_playing(vinyl, p1 ? FALSE : TRUE);
        }
        break;
    }

    case DB_EV_STOP: {
        if (vinyl) {
            vinyl_widget_set_playing(vinyl, FALSE);
        }
        break;
    }

    case DB_EV_SONGCHANGED: {
        ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
        if (ev->to && vinyl) {
            update_album_art(ev->to);
        }
        break;
    }
    }

    return 0;
}


/* ================================================================
 *  设计模式右键菜单
 * ================================================================ */

static void
vinyl_record_initmenu(ddb_gtkui_widget_t *w, GtkWidget *menu)
{
    (void)w;
    GtkWidget *mi = gtk_menu_item_new_with_label("About Vinyl Record");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    gtk_widget_show(mi);
}


/* ================================================================
 *  控件销毁回调
 * ================================================================ */

static void
vinyl_record_destroy(ddb_gtkui_widget_t *w)
{
    (void)w;
    if (vinyl) {
        vinyl_widget_set_playing(vinyl, FALSE);
        vinyl_widget_destroy(vinyl);
        vinyl = NULL;
    }
    container = NULL;
}


/* ================================================================
 *  控件工厂 —— 设计模式创建/重建控件时调用
 * ================================================================ */

static ddb_gtkui_widget_t *
vinyl_record_create(void)
{
    vinyl_record_widget_t *w = calloc(1, sizeof(vinyl_record_widget_t));
    w->base.type = "vinyl_record";

    int widget_size = deadbeef->conf_get_int("vinyl_record.size", 400);
    if (widget_size < 100) { widget_size = 100; }
    if (widget_size > 800) { widget_size = 800; }

    int cover_pct = deadbeef->conf_get_int("vinyl_record.cover_pct", 70);
    if (cover_pct < 30) { cover_pct = 30; }
    if (cover_pct > 90) { cover_pct = 90; }

    container = gtk_event_box_new();
    gtk_widget_set_size_request(container, widget_size, widget_size);

    vinyl = vinyl_widget_new(widget_size, widget_size, (double)cover_pct);
    gtk_container_add(GTK_CONTAINER(container),
                      vinyl_widget_get_widget(vinyl));

    gtk_widget_show_all(container);

    w->base.widget   = container;
    w->base.message  = vinyl_record_message;
    w->base.initmenu = vinyl_record_initmenu;
    w->base.destroy  = vinyl_record_destroy;

    gtkui_plugin_api->w_override_signals(w->base.widget, w);

    DB_playItem_t *current = deadbeef->streamer_get_playing_track();
    if (current) {
        update_album_art(current);

        struct DB_output_s *output = deadbeef->get_output();
        if (output && output->state() == DDB_PLAYBACK_STATE_PLAYING) {
            vinyl_widget_set_playing(vinyl, TRUE);
        }
        deadbeef->pl_item_unref(current);
    }

    return &w->base;
}


/* ================================================================
 *  插件动作（右键菜单 —— 备用）
 * ================================================================ */

static DB_plugin_action_t *
vinyl_record_get_actions(DB_playItem_t *it)
{
    (void)it;
    static DB_plugin_action_t actions[2];
    actions[0].title = "Show Vinyl Record";
    actions[0].name  = "show_vinyl";
    actions[0].flags = DB_ACTION_SINGLE_TRACK;
    actions[0].next  = &actions[1];
    actions[1].title = NULL;
    return actions;
}


/* ================================================================
 *  插件生命周期
 * ================================================================ */

static int
vinyl_record_start(void)
{
    return 0;
}

static int vinyl_record_disconnect(void);


static int
vinyl_record_stop(void)
{
    vinyl_record_disconnect();
    return 0;
}


/**
 * @brief 插件连接 —— 注册设计模式控件类型
 */
static int
vinyl_record_connect(void)
{
    DB_plugin_t *gtkui_plug = deadbeef->plug_get_for_id(DDB_GTKUI_PLUGIN_ID);
    if (!gtkui_plug) {
        return 0;
    }

    DB_gui_t *gui = (DB_gui_t *)gtkui_plug;
    gtkui_plugin_api = (ddb_gtkui_t *)gui;

    if (!gtkui_plugin_api->w_reg_widget) {
        return 0;
    }

    gtkui_plugin_api->w_reg_widget(
        "Vinyl Record",
        DDB_WF_SINGLE_INSTANCE,
        vinyl_record_create,
        "vinyl_record",
        NULL);

    return 0;
}


/**
 * @brief 插件断开连接 —— 注销控件类型并释放资源
 */
static int
vinyl_record_disconnect(void)
{
    if (gtkui_plugin_api && gtkui_plugin_api->w_unreg_widget) {
        gtkui_plugin_api->w_unreg_widget("vinyl_record");
    }

    if (vinyl) {
        vinyl_widget_set_playing(vinyl, FALSE);
        vinyl_widget_destroy(vinyl);
        vinyl = NULL;
    }

    gtkui_plugin_api = NULL;
    return 0;
}


/* ================================================================
 *  全局消息处理
 * ================================================================ */

static int
vinyl_record_message_global(uint32_t id, uintptr_t ctx,
                            uint32_t p1, uint32_t p2)
{
    (void)ctx;
    (void)p1;
    (void)p2;

    switch (id) {
    case DB_EV_ACTIVATED:
        break;
    }

    return 0;
}


/* ================================================================
 *  插件定义
 * ================================================================ */

static DB_misc_t plugin = {
    DDB_REQUIRE_API_VERSION(1, 10)
    .plugin.type           = DB_PLUGIN_MISC,
    .plugin.version_major  = 1,
    .plugin.version_minor  = 0,
    .plugin.flags          = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.id             = "vinyl_record",
    .plugin.name           = "Vinyl Record",
    .plugin.descr          = "Shows album art as a rotating vinyl record\n",
    .plugin.copyright      =
        "MIT License\n"
        "Copyright (c) 2026 Jiangxing Zhao\n"
        "\n"
        "Permission is hereby granted, free of charge, to any person "
        "obtaining a copy of this software and associated documentation "
        "files (the \"Software\"), to deal in the Software without "
        "restriction, including without limitation the rights to use, "
        "copy, modify, merge, publish, distribute, sublicense, and/or "
        "sell copies of the Software, and to permit persons to whom the "
        "Software is furnished to do so, subject to the following "
        "conditions:\n"
        "\n"
        "The above copyright notice and this permission notice shall be "
        "included in all copies or substantial portions of the Software.\n"
        "\n"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY "
        "KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE "
        "WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE "
        "AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT "
        "HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, "
        "WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING "
        "FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR "
        "OTHER DEALINGS IN THE SOFTWARE.\n",
    .plugin.website        = "mailto:jiangxingzhao@hotmail.com",
    .plugin.start          = vinyl_record_start,
    .plugin.connect        = vinyl_record_connect,
    .plugin.disconnect     = vinyl_record_disconnect,
    .plugin.stop           = vinyl_record_stop,
    .plugin.message        = vinyl_record_message_global,
    .plugin.get_actions    = vinyl_record_get_actions,
    .plugin.configdialog   =
        "property \"\" vbox[2] height=80 spacing=8;\n"
        "property \"Widget size (px)\" entry vinyl_record.size 400;\n"
        "property \"Cover area (%)\" entry vinyl_record.cover_pct 70;\n",
};


EXPORT DB_plugin_t *
vinyl_record_load(DB_functions_t *api)
{
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
