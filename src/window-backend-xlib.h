#ifndef _RUNES_WINDOW_BACKEND_XLIB_H
#define _RUNES_WINDOW_BACKEND_XLIB_H

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
    RUNES_ATOM_TARGETS,
    RUNES_ATOM_RUNES_FLUSH,
    RUNES_ATOM_RUNES_SELECTION,
    RUNES_NUM_ATOMS
};

struct runes_window_backend {
    Display *dpy;
    Atom atoms[RUNES_NUM_ATOMS];
};

RunesWindowBackend *runes_window_backend_new(void);
void runes_window_backend_delete(RunesWindowBackend *wb);

#endif
