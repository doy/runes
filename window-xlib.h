#ifndef _RUNES_WINDOW_XLIB_H
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

typedef struct {
    RunesLoopData data;
    XEvent e;
} RunesXlibLoopData;

void runes_window_backend_init(RunesTerm *t, int argc, char *argv[]);
cairo_surface_t *runes_window_backend_surface_create(RunesTerm *t);
void runes_window_backend_flush(RunesTerm *t);
void runes_window_backend_cleanup(RunesTerm *t);

#endif
