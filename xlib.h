#ifndef _RUNES_XLIB_H
#define _RUNES_XLIB_H

#include <X11/Xlib.h>

enum runes_atoms {
    RUNES_ATOM_WM_DELETE_WINDOW,
    RUNES_ATOM_NET_WM_PING,
    RUNES_NUM_PROTOCOL_ATOMS,
    RUNES_ATOM_NET_WM_PID = 2,
    RUNES_ATOM_NET_WM_ICON_NAME,
    RUNES_ATOM_NET_WM_NAME,
    RUNES_ATOM_UTF8_STRING,
    RUNES_NUM_ATOMS
};

struct runes_window {
    Display *dpy;
    Window w;
    GC gc;
    XIC ic;

    Atom atoms[RUNES_NUM_ATOMS];
};

struct xlib_loop_data {
    struct loop_data data;
    XEvent e;
};

RunesWindow *runes_window_create(int argc, char *argv[]);
cairo_surface_t *runes_surface_create(RunesTerm *t);
void runes_loop_init(RunesTerm *t, uv_loop_t *loop);
void runes_window_destroy(RunesWindow *w);

#endif
