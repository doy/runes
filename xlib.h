#ifndef _RUNES_XLIB_H
#define _RUNES_XLIB_H

#include <X11/Xlib.h>

struct runes_window {
    Display *dpy;
    Window w;
    GC gc;
    XIC ic;
};

RunesWindow *runes_window_create();
cairo_surface_t *runes_surface_create(RunesWindow *w);
void runes_loop_init(RunesTerm *t, uv_loop_t *loop);
void runes_window_destroy(RunesWindow *w);

#endif
