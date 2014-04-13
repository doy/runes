#include <cairo-xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "runes.h"

static char *atom_names[RUNES_NUM_ATOMS] = {
    "WM_DELETE_WINDOW",
    "_NET_WM_PING",
    "_NET_WM_PID",
    "_NET_WM_ICON_NAME",
    "_NET_WM_NAME",
    "UTF8_STRING",
    "WM_PROTOCOLS",
    "RUNES_FLUSH"
};

static void runes_window_backend_get_next_event(uv_work_t *req);
static void runes_window_backend_process_event(uv_work_t *req, int status);
static void runes_window_backend_map_window(RunesTerm *t);
static void runes_window_backend_init_wm_properties(
    RunesTerm *t, int argc, char *argv[]);
static void runes_window_backend_resize_window(
    RunesTerm *t, int width, int height);
static void runes_window_backend_flush(RunesTerm *t);

void runes_window_backend_init(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;
    unsigned long white;
    XIM im;

    XInitThreads();

    w->dpy = XOpenDisplay(NULL);
    white  = WhitePixel(w->dpy, DefaultScreen(w->dpy));
    w->w   = XCreateSimpleWindow(
        w->dpy, DefaultRootWindow(w->dpy),
        0, 0, 240, 80, 0, white, white
    );

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

    runes_window_backend_map_window(t);
}

void runes_window_backend_loop_init(RunesTerm *t, int argc, char *argv[])
{
    RunesWindowBackend *w = &t->w;
    unsigned long mask;
    void *data;

    runes_window_backend_init_wm_properties(t, argc, argv);

    XGetICValues(w->ic, XNFilterEvents, &mask, NULL);
    XSelectInput(
        w->dpy, w->w, mask|KeyPressMask|StructureNotifyMask|ExposureMask);
    XSetICFocus(w->ic);

    data = malloc(sizeof(RunesXlibLoopData));
    ((RunesLoopData *)data)->req.data = data;
    ((RunesLoopData *)data)->t = t;

    uv_queue_work(
        t->loop, data,
        runes_window_backend_get_next_event,
        runes_window_backend_process_event);
}

cairo_surface_t *runes_window_backend_surface_create(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;
    Visual *vis;
    XWindowAttributes attrs;

    XGetWindowAttributes(w->dpy, w->w, &attrs);
    vis = DefaultVisual(w->dpy, DefaultScreen(w->dpy));
    return cairo_xlib_surface_create(
        w->dpy, w->w, vis, attrs.width, attrs.height);
}

void runes_window_backend_request_flush(RunesTerm *t)
{
    XEvent e;

    e.xclient.type = ClientMessage;
    e.xclient.window = t->w.w;
    e.xclient.format = 32;
    e.xclient.data.l[0] = t->w.atoms[RUNES_ATOM_RUNES_FLUSH];

    XSendEvent(t->w.dpy, t->w.w, False, NoEventMask, &e);
    XLockDisplay(t->w.dpy);
    XFlush(t->w.dpy);
    XUnlockDisplay(t->w.dpy);
}

void runes_window_backend_get_size(RunesTerm *t, int *xpixel, int *ypixel)
{
    cairo_surface_t *surface;

    surface = cairo_get_target(t->backend_cr);
    *xpixel = cairo_xlib_surface_get_width(surface);
    *ypixel = cairo_xlib_surface_get_height(surface);
}

void runes_window_backend_set_icon_name(RunesTerm *t, char *name, size_t len)
{
    RunesWindowBackend *w = &t->w;

    XChangeProperty(
        w->dpy, w->w, XA_WM_ICON_NAME,
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
    XChangeProperty(
        w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_ICON_NAME],
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
}

void runes_window_backend_set_window_title(
    RunesTerm *t, char *name, size_t len)
{
    RunesWindowBackend *w = &t->w;

    XChangeProperty(
        w->dpy, w->w, XA_WM_NAME,
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
    XChangeProperty(
        w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_NAME],
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
}

void runes_window_backend_request_close(RunesTerm *t)
{
    XEvent e;

    e.xclient.type = ClientMessage;
    e.xclient.window = t->w.w;
    e.xclient.message_type = t->w.atoms[RUNES_ATOM_WM_PROTOCOLS];
    e.xclient.format = 32;
    e.xclient.data.l[0] = t->w.atoms[RUNES_ATOM_WM_DELETE_WINDOW];
    e.xclient.data.l[1] = CurrentTime;

    XSendEvent(t->w.dpy, t->w.w, False, NoEventMask, &e);
}

void runes_window_backend_cleanup(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;
    XIM im;

    im = XIMOfIC(w->ic);
    XDestroyIC(w->ic);
    XCloseIM(im);
    XDestroyWindow(w->dpy, w->w);
    XCloseDisplay(w->dpy);
}

static void runes_window_backend_get_next_event(uv_work_t *req)
{
    RunesXlibLoopData *data;

    data = (RunesXlibLoopData *)req->data;
    XNextEvent(data->data.t->w.dpy, &data->e);
}

static void runes_window_backend_process_event(uv_work_t *req, int status)
{
    RunesXlibLoopData *data = req->data;
    XEvent *e = &data->e;
    RunesTerm *t = data->data.t;
    RunesWindowBackend *w = &t->w;
    int should_close = 0;

    UNUSED(status);

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
                chars = Xutf8LookupString(
                    w->ic, &e->xkey, buf, len - 1, &sym, &s);
                if (s == XBufferOverflow) {
                    len = chars + 1;
                    buf = realloc(buf, len);
                    continue;
                }
                break;
            }

            if (chars) {
                runes_pty_backend_write(t, buf, chars);
            }
            else if (sym) {
                switch (sym) {
                case XK_Up:
                    runes_pty_backend_write(t, "\e[A", 3);
                    break;
                case XK_Down:
                    runes_pty_backend_write(t, "\e[B", 3);
                    break;
                case XK_Right:
                    runes_pty_backend_write(t, "\e[C", 3);
                    break;
                case XK_Left:
                    runes_pty_backend_write(t, "\e[D", 3);
                    break;
                }
            }
            free(buf);
            break;
        }
        case Expose:
            runes_window_backend_flush(t);
            break;
        case ConfigureNotify:
            while (XCheckTypedWindowEvent(w->dpy, w->w, ConfigureNotify, e));
            runes_window_backend_resize_window(
                t, e->xconfigure.width, e->xconfigure.height);
            break;
        case ClientMessage: {
            Atom a = e->xclient.data.l[0];
            if (a == w->atoms[RUNES_ATOM_WM_DELETE_WINDOW]) {
                should_close = 1;
            }
            else if (a == w->atoms[RUNES_ATOM_NET_WM_PING]) {
                e->xclient.window = DefaultRootWindow(w->dpy);
                XSendEvent(
                    w->dpy, e->xclient.window, False,
                    SubstructureNotifyMask | SubstructureRedirectMask,
                    e
                );
            }
            else if (a == w->atoms[RUNES_ATOM_RUNES_FLUSH]) {
                runes_window_backend_flush(t);
            }
            break;
        }
        default:
            break;
        }
    }

    if (!should_close) {
        uv_queue_work(
            t->loop, req, runes_window_backend_get_next_event,
            runes_window_backend_process_event);
    }
    else {
        runes_pty_backend_request_close(t);
        free(req);
    }
}

static void runes_window_backend_map_window(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    XSelectInput(w->dpy, w->w, StructureNotifyMask);
    XMapWindow(w->dpy, w->w);

    for (;;) {
        XEvent e;

        XNextEvent(w->dpy, &e);
        if (e.type == MapNotify) {
            break;
        }
    }
}

static void runes_window_backend_init_wm_properties(
    RunesTerm *t, int argc, char *argv[])
{
    RunesWindowBackend *w = &t->w;
    pid_t pid;
    XClassHint class_hints = { "runes", "runes" };
    XWMHints wm_hints;
    XSizeHints normal_hints;

    wm_hints.flags = InputHint | StateHint;
    wm_hints.input = True;
    wm_hints.initial_state = NormalState;

    /* XXX */
    normal_hints.flags = 0;

    XInternAtoms(w->dpy, atom_names, RUNES_NUM_ATOMS, False, w->atoms);
    XSetWMProtocols(w->dpy, w->w, w->atoms, RUNES_NUM_PROTOCOL_ATOMS);

    Xutf8SetWMProperties(
        w->dpy, w->w, "runes", "runes", argv, argc,
        &normal_hints, &wm_hints, &class_hints);

    pid = getpid();
    XChangeProperty(
        w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_PID],
        XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&pid, 1);

    runes_window_backend_set_icon_name(t, "runes", 5);
    runes_window_backend_set_window_title(t, "runes", 5);
}

static void runes_window_backend_resize_window(
    RunesTerm *t, int width, int height)
{
    /* XXX no idea why shrinking the window dimensions to 0 makes xlib think
     * that the dimension is 65535 */
    if (width < 1 || width >= 65535) {
        width = 1;
    }
    if (height < 1 || height >= 65535) {
        height = 1;
    }

    if (width != t->xpixel || height != t->ypixel) {
        cairo_xlib_surface_set_size(
            cairo_get_target(t->backend_cr), width, height);
        runes_display_set_window_size(t, width, height);
        runes_pty_backend_set_window_size(t);
    }
}

static void runes_window_backend_flush(RunesTerm *t)
{
    cairo_set_source_surface(t->backend_cr, cairo_get_target(t->cr), 0.0, 0.0);
    cairo_paint(t->backend_cr);
    runes_display_draw_cursor(t);
    XFlush(t->w.dpy);
}
