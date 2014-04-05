#ifndef _RUNES_XLIB_H
#define _RUNES_XLIB_H

#include <X11/Xlib.h>

typedef struct {
    Display *dpy;
    Window w;
    GC gc;
    XIC ic;
} RunesWindow;

RunesWindow *runes_window_create();
cairo_surface_t *runes_surface_create(RunesWindow *w);
void runes_window_prepare_input(RunesWindow *w);
void runes_window_read_key(RunesWindow *w, char **buf, size_t *len);
void runes_window_destroy(RunesWindow *w);

#endif
