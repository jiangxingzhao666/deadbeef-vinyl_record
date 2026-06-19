/**
 * @file vinyl_widget.c
 * @brief 黑胶唱片绘制控件 —— 实现
 *
 * 使用 Cairo 绘制引擎实现以下视觉效果：
 *   - 黑色唱片基底 + 30 圈同心凹槽纹理（模拟真实唱片）
 *   - 弧形高光（模拟光泽反射）
 *   - 中心圆形专辑封面（随唱片旋转）
 *   - 默认黑色标签 + 中心孔（无封面时）
 *
 * 旋转动画：
 *   - GLib 定时器每 ~33ms 触发一次重绘
 *   - 每帧旋转 1°，帧率约 30fps
 */

#include "vinyl_widget.h"
#include <math.h>
#include <stdlib.h>

#define TIMER_INTERVAL 33
#define ROTATION_SPEED 1.0


/* ================================================================
 *  内部函数声明
 * ================================================================ */

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean on_timer(gpointer data);


/* ================================================================
 *  创建
 * ================================================================ */

VinylWidget *
vinyl_widget_new(int width, int height, double cover_pct)
{
    VinylWidget *vw = calloc(1, sizeof(VinylWidget));
    vw->width          = width;
    vw->height         = height;
    vw->rotation_angle = 0.0;
    vw->is_playing     = FALSE;
    vw->album_art      = NULL;
    vw->cover_pct      = cover_pct;

    vw->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(vw->drawing_area, width, height);

    g_signal_connect(vw->drawing_area, "draw", G_CALLBACK(on_draw), vw);

    gtk_widget_queue_draw(vw->drawing_area);
    return vw;
}


GtkWidget *
vinyl_widget_get_widget(VinylWidget *vw)
{
    return vw->drawing_area;
}


/* ================================================================
 *  专辑封面设置
 * ================================================================ */

void
vinyl_widget_set_album_art(VinylWidget *vw, cairo_surface_t *art)
{
    if (art == vw->album_art) {
        gtk_widget_queue_draw(vw->drawing_area);
        return;
    }

    if (vw->album_art) {
        cairo_surface_destroy(vw->album_art);
    }

    vw->album_art = art;
    if (art) {
        cairo_surface_reference(art);
    }

    gtk_widget_queue_draw(vw->drawing_area);
}


/* ================================================================
 *  播放状态控制（旋转动画开关）
 * ================================================================ */

void
vinyl_widget_set_playing(VinylWidget *vw, gboolean playing)
{
    if (playing && !vw->is_playing) {
        vw->is_playing = TRUE;
        vw->timer_id = g_timeout_add(TIMER_INTERVAL, on_timer, vw);
    } else if (!playing && vw->is_playing) {
        vw->is_playing = FALSE;
        if (vw->timer_id) {
            g_source_remove(vw->timer_id);
            vw->timer_id = 0;
        }
    }
}


/* ================================================================
 *  定时器回调 —— 更新旋转角度并触发重绘
 * ================================================================ */

static gboolean
on_timer(gpointer data)
{
    VinylWidget *vw = (VinylWidget *)data;

    vw->rotation_angle += ROTATION_SPEED;
    if (vw->rotation_angle >= 360.0) {
        vw->rotation_angle -= 360.0;
    }

    gtk_widget_queue_draw(vw->drawing_area);
    return G_SOURCE_CONTINUE;
}


/* ================================================================
 *  Cairo 绘制回调 —— 渲染整个唱片
 * ================================================================ */

/**
 * @brief GTK draw 信号回调
 *
 * 绘制层次（从底到顶）：
 *   1. 唱片阴影（不旋转）
 *   2. 旋转组（translate + rotate）：
 *      a. 黑色唱片基底
 *      b. 30 圈同心凹槽纹理
 *      c. 弧形高光
 *      d. 专辑封面或默认黑色标签
 *   3. 中心孔
 */
static gboolean
on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    VinylWidget *vw = (VinylWidget *)data;

    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);

    double cx      = w / 2.0;
    double cy      = h / 2.0;
    double outer_r = MIN(cx, cy) * 0.90;
    double label_r = outer_r * (vw->cover_pct / 100.0);


    /* ---- 第1层：唱片阴影 ---- */
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.35);
    cairo_arc(cr, cx + 3, cy + 3, outer_r, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);


    /* ==== 旋转组 ==== */
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, vw->rotation_angle * M_PI / 180.0);


    /* ---- 第2层：黑色唱片基底 ---- */
    cairo_arc(cr, 0, 0, outer_r, 0, 2 * M_PI);
    cairo_set_source_rgb(cr, 0.08, 0.08, 0.08);
    cairo_fill(cr);


    /* ---- 第3层：凹槽纹理（30圈同心圆） ---- */
    double groove_start = (vw->cover_pct / 100.0) + 0.03;
    double groove_span  = 0.98 - groove_start;
    for (int i = 0; i < 30; i++) {
        double groove_r = outer_r * (groove_start + groove_span * i / 30.0);
        cairo_arc(cr, 0, 0, groove_r, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, 0.18, 0.18, 0.18, 0.15);
        cairo_set_line_width(cr, 0.8);
        cairo_stroke(cr);
    }


    /* ---- 第4层：弧形高光 ---- */
    cairo_arc(cr, 0, 0, outer_r, -M_PI / 3, M_PI / 3);
    cairo_set_source_rgba(cr, 0.22, 0.22, 0.22, 0.3);
    cairo_set_line_width(cr, outer_r * 0.3);
    cairo_stroke(cr);


    /* ---- 第5层：中心封面或默认标签 ---- */
    if (vw->album_art) {

        cairo_arc(cr, 0, 0, label_r, 0, 2 * M_PI);
        cairo_clip(cr);

        int art_w = cairo_image_surface_get_width(vw->album_art);
        int art_h = cairo_image_surface_get_height(vw->album_art);
        double scale = (label_r * 2) / MIN(art_w, art_h);

        double art_x = -art_w * scale / 2.0;
        double art_y = -art_h * scale / 2.0;

        cairo_save(cr);
        cairo_translate(cr, art_x, art_y);
        cairo_scale(cr, scale, scale);
        cairo_set_source_surface(cr, vw->album_art, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);

        cairo_reset_clip(cr);
        cairo_arc(cr, 0, 0, label_r, 0, 2 * M_PI);
        cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
        cairo_set_line_width(cr, 2);
        cairo_stroke(cr);

    } else {

        cairo_arc(cr, 0, 0, label_r, 0, 2 * M_PI);
        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        cairo_fill(cr);

        cairo_arc(cr, 0, 0, label_r, 0, 2 * M_PI);
        cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
        cairo_set_line_width(cr, 2);
        cairo_stroke(cr);

        double ring_r = outer_r * 0.048;
        cairo_arc(cr, 0, 0, ring_r, 0, 2 * M_PI);
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_fill(cr);
    }


    cairo_restore(cr);  /* 结束旋转组 */


    /* ---- 第6层：中心孔 ---- */
    double hole_r = outer_r * 0.032;
    cairo_arc(cr, cx, cy, hole_r, 0, 2 * M_PI);
    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_fill(cr);

    return TRUE;
}


/* ================================================================
 *  销毁
 * ================================================================ */

void
vinyl_widget_destroy(VinylWidget *vw)
{
    if (vw->timer_id) {
        g_source_remove(vw->timer_id);
    }
    if (vw->album_art) {
        cairo_surface_destroy(vw->album_art);
    }
    free(vw);
}
