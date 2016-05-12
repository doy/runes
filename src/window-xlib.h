#ifndef _RUNES_WINDOW_XLIB_H
#define _RUNES_WINDOW_XLIB_H

#include <cairo.h>
#include <time.h>
#include <X11/Xlib.h>

struct runes_window {
    RunesWindowBackend *wb;
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

RunesWindow *runes_window_new(RunesWindowBackend *wb);
void runes_window_create_window(RunesTerm *t, int argc, char *argv[]);
void runes_window_init_loop(RunesTerm *t, RunesLoop *loop);
void runes_window_request_flush(RunesTerm *t);
void runes_window_request_close(RunesTerm *t);
unsigned long runes_window_get_window_id(RunesTerm *t);
void runes_window_get_size(RunesTerm *t, int *xpixel, int *ypixel);
void runes_window_set_icon_name(RunesTerm *t, char *name, size_t len);
void runes_window_set_window_title(RunesTerm *t, char *name, size_t len);
void runes_window_delete(RunesWindow *w);

#endif
