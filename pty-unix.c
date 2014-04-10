#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "runes.h"

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

        execl(shell, shell, (char *)NULL);
    }
}

static void runes_read_pty(uv_work_t *req)
{
    RunesPtyLoopData *data;

    data = (RunesPtyLoopData *)req->data;
    data->len = read(data->data.t->pty.master, data->buf, RUNES_PTY_BUFFER_LENGTH);
}

static void runes_got_pty_data(uv_work_t *req, int status)
{
    RunesTerm *t;
    RunesPtyLoopData *data;

    UNUSED(status);
    data = (RunesPtyLoopData *)req->data;
    t = data->data.t;

    if (data->len > 0) {
        runes_handle_pty_read(t, data->buf, data->len);
        uv_queue_work(t->loop, req, runes_read_pty, runes_got_pty_data);
    }
    else {
        runes_handle_pty_close(t);
        free(req);
    }
}

void runes_pty_backend_loop_init(RunesTerm *t)
{
    void *data;

    data = malloc(sizeof(RunesPtyLoopData));
    ((RunesLoopData *)data)->req.data = data;
    ((RunesLoopData *)data)->t = t;

    uv_queue_work(t->loop, data, runes_read_pty, runes_got_pty_data);
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
