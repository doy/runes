#include <cairo-xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "runes.h"

static char *atom_names[RUNES_NUM_ATOMS] = {
    "WM_DELETE_WINDOW"
};

RunesWindow *runes_window_create()
{
    RunesWindow *w;
    unsigned long white;
    XIM im;

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

    XSetLocaleModifiers("");
    im = XOpenIM(w->dpy, NULL, NULL, NULL);
    w->ic = XCreateIC(
        im,
        XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, w->w,
        XNFocusWindow, w->w,
        NULL
    );
    if (w->ic == NULL) {
        fprintf(stderr, "failed\n");
        exit(1);
    }

    return w;
}

cairo_surface_t *runes_surface_create(RunesTerm *t)
{
    Visual *vis;
    XWindowAttributes attrs;

    XGetWindowAttributes(t->w->dpy, t->w->w, &attrs);
    vis = DefaultVisual(t->w->dpy, DefaultScreen(t->w->dpy));
    return cairo_xlib_surface_create(t->w->dpy, t->w->w, vis, attrs.width, attrs.height);
}

static void runes_get_next_event(uv_work_t *req)
{
    struct xlib_loop_data *data;

    data = (struct xlib_loop_data *)req->data;
    XNextEvent(data->data.t->w->dpy, &data->e);
}

static void runes_process_event(uv_work_t *req, int status)
{
    struct xlib_loop_data *data;
    XEvent *e;
    RunesTerm *t;

    UNUSED(status);

    data = ((struct xlib_loop_data *)req->data);
    e = &data->e;
    t = data->data.t;

    if (!XFilterEvent(e, None)) {
        switch (e->type) {
        case KeyPress: {
            char *buf = NULL;
            size_t len = 8;
            KeySym sym;
            Status s;
            size_t chars;

            buf = malloc(len);

            for (;;) {
                chars = Xutf8LookupString(t->w->ic, &e->xkey, buf, len - 1, &sym, &s);
                if (s == XBufferOverflow) {
                    len = chars + 1;
                    buf = realloc(buf, len);
                    continue;
                }
                break;
            }

            runes_handle_keyboard_event(t, buf, chars);
            free(buf);
            break;
        }
        case ClientMessage:
            if ((Atom)e->xclient.data.l[0] == t->w->atoms[RUNES_ATOM_WM_DELETE_WINDOW]) {
                runes_handle_close_window(t);
            }
            break;
        default:
            break;
        }
    }

    if (t->loop) {
        uv_queue_work(t->loop, req, runes_get_next_event, runes_process_event);
    }
}

void runes_loop_init(RunesTerm *t, uv_loop_t *loop)
{
    unsigned long mask;
    void *data;

    XGetICValues(t->w->ic, XNFilterEvents, &mask, NULL);
    XSelectInput(t->w->dpy, t->w->w, mask|KeyPressMask|StructureNotifyMask);
    XSetICFocus(t->w->ic);

    data = malloc(sizeof(struct xlib_loop_data));
    ((struct loop_data *)data)->req.data = data;
    ((struct loop_data *)data)->t = t;

    XInternAtoms(t->w->dpy, atom_names, RUNES_NUM_ATOMS, False, t->w->atoms);
    XSetWMProtocols(t->w->dpy, t->w->w, &t->w->atoms[RUNES_ATOM_WM_DELETE_WINDOW], 1);

    uv_queue_work(loop, data, runes_get_next_event, runes_process_event);
}

void runes_window_destroy(RunesWindow *w)
{
    XIM im;

    im = XIMOfIC(w->ic);
    XDestroyIC(w->ic);
    XCloseIM(im);
    XFreeGC(w->dpy, w->gc);
    XDestroyWindow(w->dpy, w->w);
    XCloseDisplay(w->dpy);

    free(w);
}
