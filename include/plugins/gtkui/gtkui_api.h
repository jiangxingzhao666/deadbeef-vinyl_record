#ifndef __GTKUI_API_H
#define __GTKUI_API_H

#include <stdint.h>
#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(3,0,0)
#define DDB_GTKUI_PLUGIN_ID "gtkui3_1"
#else
#define DDB_GTKUI_PLUGIN_ID "gtkui_1"
#endif

#define DDB_GTKUI_API_VERSION_MAJOR 2
#define DDB_GTKUI_API_VERSION_MINOR 6

#ifdef __GNUC__
#if defined __GNUC__ && defined __GNUC_MINOR__
# define __GNUC_PREREQ(maj, min) \
    ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define __GNUC_PREREQ(maj, min) 0
#endif
#endif

enum {
    DDB_WF_SINGLE_INSTANCE = 1<<0,
    DDB_WF_SUPPORTS_EXTENDED_API = 1<<1,
};

typedef struct ddb_gtkui_widget_s {
    const char *type;
    struct ddb_gtkui_widget_s *parent;
    GtkWidget *widget;
    uint32_t flags;

    void (*init) (struct ddb_gtkui_widget_s *container);
    void (*save) (struct ddb_gtkui_widget_s *w, char *s, int sz);
    const char *(*load) (struct ddb_gtkui_widget_s *w, const char *type, const char *s);
    void (*destroy) (struct ddb_gtkui_widget_s *w);
    void (*append) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child);
    void (*remove) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child);
    void (*replace) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child, struct ddb_gtkui_widget_s *newchild);
    GtkWidget * (*get_container) (struct ddb_gtkui_widget_s *w);
    int (*message) (struct ddb_gtkui_widget_s *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);
    void (*initmenu) (struct ddb_gtkui_widget_s *w, GtkWidget *menu);
    void (*initchildmenu) (struct ddb_gtkui_widget_s *w, GtkWidget *menu);

    struct ddb_gtkui_widget_s *children;
    struct ddb_gtkui_widget_s *next;
} ddb_gtkui_widget_t;

typedef struct {
    DB_gui_t gui;

    GtkWidget * (*get_mainwin) (void);

    void (*w_reg_widget) (const char *title, uint32_t flags, ddb_gtkui_widget_t *(*create_func) (void), ...);

    void (*w_unreg_widget) (const char *type);

    void (*w_override_signals) (GtkWidget *w, gpointer user_data);

    int (*w_is_registered) (const char *type);

    ddb_gtkui_widget_t * (*w_get_rootwidget) (void);

    void (*w_set_design_mode) (int active);

    int (*w_get_design_mode) (void);

    ddb_gtkui_widget_t * (*w_create) (const char *type);

    void (*w_destroy) (ddb_gtkui_widget_t *w);

    void (*w_append) (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

    void (*w_replace) (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *from, ddb_gtkui_widget_t *to);

    void (*w_remove) (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

    GtkWidget * (*w_get_container) (ddb_gtkui_widget_t *w);

    GtkWidget* (*create_pltmenu) (int plt_idx);

    GdkPixbuf *(*get_cover_art_pixbuf) (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data);
    GdkPixbuf *(*cover_get_default_pixbuf) (void);
} ddb_gtkui_t;

#endif
