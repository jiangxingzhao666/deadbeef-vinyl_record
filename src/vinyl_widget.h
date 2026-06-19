/**
 * @file vinyl_widget.h
 * @brief 黑胶唱片绘制控件 —— 公共接口
 *
 * 该控件封装了一个 GTK 绘制区域，将专辑封面渲染为
 * 具有凹槽纹理的黑胶唱片样式。支持旋转动画（播放时）。
 */

#ifndef VINYL_WIDGET_H
#define VINYL_WIDGET_H

#include <gtk/gtk.h>
#include <cairo.h>

/**
 * @struct VinylWidget
 * @brief  唱片控件状态数据
 *
 * 持有 GTK 绘制区域、专辑封面 surface、旋转角度、
 * 播放状态和动画定时器等全部状态。
 */
typedef struct {
    GtkWidget       *drawing_area;
    cairo_surface_t *album_art;
    gdouble          rotation_angle;
    gboolean         is_playing;
    guint            timer_id;
    int              width;
    int              height;
    double           cover_pct;
} VinylWidget;

VinylWidget *vinyl_widget_new(int width, int height, double cover_pct);
GtkWidget   *vinyl_widget_get_widget(VinylWidget *vw);
void         vinyl_widget_set_album_art(VinylWidget *vw, cairo_surface_t *art);
void         vinyl_widget_set_playing(VinylWidget *vw, gboolean playing);
void         vinyl_widget_destroy(VinylWidget *vw);

#endif
