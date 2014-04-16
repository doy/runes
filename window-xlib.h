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
    RUNES_ATOM_WM_PROTOCOLS,
    RUNES_ATOM_RUNES_FLUSH,
    RUNES_NUM_ATOMS
};

struct runes_window {
    Display *dpy;
    Window w;
    XIC ic;

    Atom atoms[RUNES_NUM_ATOMS];
};

typedef struct {
    RunesLoopData data;
    XEvent e;
} RunesXlibLoopData;

void runes_window_backend_create_window(RunesTerm *t, int argc, char *argv[]);
void runes_window_backend_start_loop(RunesTerm *t);
void runes_window_backend_request_flush(RunesTerm *t);
void runes_window_backend_request_close(RunesTerm *t);
void runes_window_backend_get_size(RunesTerm *t, int *xpixel, int *ypixel);
void runes_window_backend_set_icon_name(RunesTerm *t, char *name, size_t len);
void runes_window_backend_set_window_title(
    RunesTerm *t, char *name, size_t len);
void runes_window_backend_visual_bell(RunesTerm *t);
void runes_window_backend_cleanup(RunesTerm *t);

#endif
