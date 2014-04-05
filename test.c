#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
    Display *dpy;
    unsigned long white;
    Window w;
    Visual *vis;
    GC gc;

    cairo_surface_t *surface;
    cairo_t *cr;

    dpy   = XOpenDisplay(NULL);
    white = WhitePixel(dpy, DefaultScreen(dpy));
    w     = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 240, 80, 0, white, white);
    vis   = DefaultVisual(dpy, DefaultScreen(dpy));

    XSelectInput(dpy, w, StructureNotifyMask);
    XMapWindow(dpy, w);
    gc = XCreateGC(dpy, w, 0, NULL);
    XSetForeground(dpy, gc, white);

    for (;;) {
        XEvent e;

        XNextEvent(dpy, &e);
        if (e.type == MapNotify) {
            break;
        }
    }

    surface = cairo_xlib_surface_create(dpy, w, vis, 240, 80);
    cr      = cairo_create(surface);

    cairo_select_font_face(cr, "7x14", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 14.0);
    cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
    cairo_move_to(cr, 10.0, 50.0);
    cairo_show_text(cr, "Hello, world");

    XFlush(dpy);

    sleep(2);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    XDestroyWindow(dpy, w);
    XCloseDisplay(dpy);

    return 0;
}
