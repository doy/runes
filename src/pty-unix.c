#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "runes.h"

static void runes_pty_backend_read(uv_work_t *req);
static void runes_pty_backend_got_data(uv_work_t *req, int status);

void runes_pty_backend_spawn_subprocess(RunesTerm *t)
{
    RunesPtyBackend *pty = &t->pty;

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
        char *cmd;
        int old_stderr_fd;
        FILE *old_stderr;

        old_stderr_fd = dup(2);
        fcntl(old_stderr_fd, F_SETFD, FD_CLOEXEC);
        old_stderr = fdopen(old_stderr_fd, "w");

        setsid();
        ioctl(pty->slave, TIOCSCTTY, NULL);

        close(pty->master);

        dup2(pty->slave, 0);
        dup2(pty->slave, 1);
        dup2(pty->slave, 2);

        close(pty->slave);

        cmd = t->cmd;
        if (!cmd) {
            cmd = getenv("SHELL");
        }
        if (!cmd) {
            cmd = "/bin/sh";
        }

        /* XXX should use a different TERM value eventually, but for right now
         * screen is compatible enough */
        setenv("TERM", "screen", 1);
        /* gnome-terminal sets this, so avoid confusing applications which
         * introspect it. not setting it to something else because as far as i
         * can tell, it's not actually useful these days, given that terminfo
         * databases are much more reliable than they were 10 years ago */
        unsetenv("COLORTERM");

        unsetenv("LINES");
        unsetenv("COLUMNS");

        if (strchr(cmd, ' ')) {
            execlp("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
        }
        else {
            execlp(cmd, cmd, (char *)NULL);
        }

        fprintf(old_stderr, "Couldn't run %s: %s\n", cmd, strerror(errno));
        exit(1);
    }
}

void runes_pty_backend_start_loop(RunesTerm *t)
{
    void *data;

    data = malloc(sizeof(RunesLoopData));
    ((RunesLoopData *)data)->req.data = data;
    ((RunesLoopData *)data)->t = t;

    uv_queue_work(
        t->loop, data, runes_pty_backend_read, runes_pty_backend_got_data);
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

void runes_pty_backend_write(RunesTerm *t, char *buf, size_t len)
{
    write(t->pty.master, buf, len);
}

void runes_pty_backend_request_close(RunesTerm *t)
{
    RunesPtyBackend *pty = &t->pty;

    kill(pty->child_pid, SIGHUP);
}

void runes_pty_backend_cleanup(RunesTerm *t)
{
    RunesPtyBackend *pty = &t->pty;

    close(pty->master);
}

static void runes_pty_backend_read(uv_work_t *req)
{
    RunesLoopData *data = req->data;
    RunesTerm *t = data->t;

    runes_window_backend_request_flush(t);
    t->readlen = read(
        t->pty.master, t->readbuf + t->remaininglen,
        RUNES_READ_BUFFER_LENGTH - t->remaininglen);
}

static void runes_pty_backend_got_data(uv_work_t *req, int status)
{
    RunesLoopData *data = req->data;
    RunesTerm *t = data->t;

    UNUSED(status);

    if (t->readlen > 0) {
        runes_parser_process_string(
            t, t->readbuf, t->readlen + t->remaininglen);
        uv_queue_work(
            t->loop, req, runes_pty_backend_read, runes_pty_backend_got_data);
    }
    else {
        runes_window_backend_request_close(t);
        free(req);
    }
}
