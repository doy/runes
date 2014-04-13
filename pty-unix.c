#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "runes.h"

static void runes_pty_backend_read(uv_work_t *req);
static void runes_pty_backend_got_data(uv_work_t *req, int status);

void runes_pty_backend_init(RunesTerm *t)
{
    RunesPtyBackend *pty;

    pty = &t->pty;
    pty->master = posix_openpt(O_RDWR);
    grantpt(pty->master);
    unlockpt(pty->master);
    pty->slave = open(ptsname(pty->master), O_RDWR);

    pty->child_pid = fork();
    if (pty->child_pid) {
        close(pty->slave);
        pty->slave = -1;
    }
    else {
        char *shell;

        setsid();
        ioctl(pty->slave, TIOCSCTTY, NULL);

        close(pty->master);

        dup2(pty->slave, 0);
        dup2(pty->slave, 1);
        dup2(pty->slave, 2);

        close(pty->slave);

        shell = getenv("SHELL");
        if (!shell) {
            shell = "/bin/sh";
        }

        setenv("TERM", "screen", 1);
        unsetenv("LINES");
        unsetenv("COLUMNS");

        execl(shell, shell, (char *)NULL);
    }
}

void runes_pty_backend_set_window_size(RunesTerm *t)
{
    struct winsize size;

    size.ws_row = t->rows;
    size.ws_col = t->cols;
    size.ws_xpixel = t->xpixel;
    size.ws_ypixel = t->ypixel;
    ioctl(t->pty.master, TIOCSWINSZ, &size);
}

void runes_pty_backend_loop_init(RunesTerm *t)
{
    void *data;

    data = malloc(sizeof(RunesPtyLoopData));
    ((RunesLoopData *)data)->req.data = data;
    ((RunesLoopData *)data)->t = t;

    runes_pty_backend_set_window_size(t);

    uv_queue_work(
        t->loop, data, runes_pty_backend_read, runes_pty_backend_got_data);
}

void runes_pty_backend_write(RunesTerm *t, char *buf, size_t len)
{
    write(t->pty.master, buf, len);
}

void runes_pty_backend_request_close(RunesTerm *t)
{
    RunesPtyBackend *pty;

    pty = &t->pty;
    kill(pty->child_pid, SIGHUP);
}

void runes_pty_backend_cleanup(RunesTerm *t)
{
    RunesPtyBackend *pty;

    pty = &t->pty;
    close(pty->master);
}

static void runes_pty_backend_read(uv_work_t *req)
{
    RunesPtyLoopData *data;

    data = (RunesPtyLoopData *)req->data;
    data->len = read(
        data->data.t->pty.master, data->buf, RUNES_PTY_BUFFER_LENGTH);
}

static void runes_pty_backend_got_data(uv_work_t *req, int status)
{
    RunesTerm *t;
    RunesPtyLoopData *data;

    UNUSED(status);
    data = (RunesPtyLoopData *)req->data;
    t = data->data.t;

    if (data->len > 0) {
        runes_vt100_process_string(t, data->buf, data->len);
        uv_queue_work(
            t->loop, req, runes_pty_backend_read, runes_pty_backend_got_data);
    }
    else {
        runes_window_backend_request_close(t);
        free(req);
    }
}
