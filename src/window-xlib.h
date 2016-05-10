#ifndef _RUNES_WINDOW_XLIB_H
#define _RUNES_WINDOW_XLIB_H

#include <cairo.h>
#include <time.h>
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

struct runes_window {
    RunesWindowBackendGlobal *wg;
    Window w;
    Window border_w;
    XIC ic;
    XEvent event;
    char *selection_contents;
    size_t selection_len;
    struct timespec last_redraw;

    cairo_t *backend_cr;

    char visual_bell_is_ringing: 1;
    char delaying: 1;
};

struct runes_window_global {
    Display *dpy;
    Atom atoms[RUNES_NUM_ATOMS];
};

RunesWindowBackendGlobal *runes_window_backend_global_init();
RunesWindowBackend *runes_window_backend_new();
void runes_window_backend_create_window(RunesTerm *t, int argc, char *argv[]);
void runes_window_backend_init_loop(RunesTerm *t, RunesLoop *loop);
void runes_window_backend_request_flush(RunesTerm *t);
void runes_window_backend_request_close(RunesTerm *t);
unsigned long runes_window_backend_get_window_id(RunesTerm *t);
void runes_window_backend_get_size(RunesTerm *t, int *xpixel, int *ypixel);
void runes_window_backend_set_icon_name(RunesTerm *t, char *name, size_t len);
void runes_window_backend_set_window_title(
    RunesTerm *t, char *name, size_t len);
void runes_window_backend_delete(RunesWindowBackend *w);
void runes_window_backend_global_cleanup(RunesWindowBackendGlobal *wg);

#endif
