#include <cairo-xlib.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "xlib.h"

RunesWindow *runes_window_create()
{
    RunesWindow *w;
    unsigned long white;

    w = malloc(sizeof(RunesWindow));

    w->dpy = XOpenDisplay(NULL);
    white  = WhitePixel(w->dpy, DefaultScreen(w->dpy));
    w->w   = XCreateSimpleWindow(
        w->dpy, DefaultRootWindow(w->dpy),
        0, 0, 240, 80, 0, white, white
    );

    XSelectInput(w->dpy, w->w, StructureNotifyMask);
    XMapWindow(w->dpy, w->w);
    w->gc = XCreateGC(w->dpy, w->w, 0, NULL);
    XSetForeground(w->dpy, w->gc, white);

    for (;;) {
        XEvent e;

        XNextEvent(w->dpy, &e);
        if (e.type == MapNotify) {
            break;
        }
    }

    return w;
}

cairo_surface_t *runes_surface_create(RunesWindow *w)
{
    Visual *vis;

    vis = DefaultVisual(w->dpy, DefaultScreen(w->dpy));
    return cairo_xlib_surface_create(w->dpy, w->w, vis, 240, 80);
}

void runes_window_flush(RunesWindow *w)
{
    XFlush(w->dpy);
}

void runes_window_destroy(RunesWindow *w)
{
    XDestroyWindow(w->dpy, w->w);
    XCloseDisplay(w->dpy);

    free(w);
}
