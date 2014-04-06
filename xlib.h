#ifndef _RUNES_XLIB_H
#define _RUNES_XLIB_H

#include <X11/Xlib.h>

struct runes_window {
    Display *dpy;
    Window w;
    GC gc;
    XIC ic;
};

struct xlib_loop_data {
    struct loop_data data;
    XEvent e;
};

RunesWindow *runes_window_create();
cairo_surface_t *runes_surface_create(RunesTerm *t);
void runes_loop_init(RunesTerm *t, uv_loop_t *loop);
void runes_window_destroy(RunesWindow *w);

#endif
