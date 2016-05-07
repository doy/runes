#define _XOPEN_SOURCE 600
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vt100.h>

#include "runes.h"
#include "pty-unix.h"

#include "config.h"
#include "loop.h"
#include "term.h"
#include "window-xlib.h"

static void runes_pty_backend_read(void *t);
static int runes_pty_backend_got_data(void *t);

void runes_pty_backend_init(RunesPtyBackend *pty)
{
    pty->master = posix_openpt(O_RDWR);
    grantpt(pty->master);
    unlockpt(pty->master);
}

void runes_pty_backend_spawn_subprocess(RunesTerm *t)
{
    RunesPtyBackend *pty = t->pty;

    pty->slave = open(ptsname(pty->master), O_RDWR);

    pty->child_pid = fork();
    if (pty->child_pid) {
        close(pty->slave);
        pty->slave = -1;
    }
    else {
        char *cmd, window_id[32];
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

        cmd = t->config->cmd;
        if (!cmd) {
            cmd = getenv("SHELL");
        }
        if (!cmd) {
            cmd = "/bin/sh";
        }

        /* XXX should use a different TERM value eventually, but for right now
         * screen is compatible enough */
        setenv("TERM", "screen-256color", 1);
        /* gnome-terminal sets this, so avoid confusing applications which
         * introspect it. not setting it to something else because as far as i
         * can tell, it's not actually useful these days, given that terminfo
         * databases are much more reliable than they were 10 years ago */
        unsetenv("COLORTERM");
        /* less sure about this one being useless, but leaving it unset for now
         * until someone complains */
        unsetenv("COLORFGBG");

        /* this is used by, for instance, w3m */
        sprintf(window_id, "%lu", runes_window_backend_get_window_id(t));
        setenv("WINDOWID", window_id, 1);

        unsetenv("LINES");
        unsetenv("COLUMNS");

        if (strpbrk(cmd, " $")) {
            execlp("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
        }
        else {
            execlp(cmd, cmd, (char *)NULL);
        }

        fprintf(old_stderr, "Couldn't run %s: %s\n", cmd, strerror(errno));
        exit(1);
    }
}

void runes_pty_backend_init_loop(RunesTerm *t, RunesLoop *loop)
{
    runes_loop_start_work(loop, t, runes_pty_backend_read,
                          runes_pty_backend_got_data);
}

void runes_pty_backend_set_window_size(RunesTerm *t, int row, int col,
                                       int xpixel, int ypixel)
{
    struct winsize size;

    size.ws_row = row;
    size.ws_col = col;
    size.ws_xpixel = xpixel;
    size.ws_ypixel = ypixel;
    ioctl(t->pty->master, TIOCSWINSZ, &size);
}

void runes_pty_backend_write(RunesTerm *t, char *buf, size_t len)
{
    write(t->pty->master, buf, len);
}

void runes_pty_backend_request_close(RunesTerm *t)
{
    RunesPtyBackend *pty = t->pty;

    kill(pty->child_pid, SIGHUP);
}

void runes_pty_backend_cleanup(RunesPtyBackend *pty)
{
    close(pty->master);
}

static void runes_pty_backend_read(void *t)
{
    RunesPtyBackend *pty = ((RunesTerm *)t)->pty;

    runes_window_backend_request_flush(t);
    pty->readlen = read(
        pty->master, pty->readbuf + pty->remaininglen,
        RUNES_READ_BUFFER_LENGTH - pty->remaininglen);
}

static int runes_pty_backend_got_data(void *t)
{
    RunesPtyBackend *pty = ((RunesTerm *)t)->pty;

    if (pty->readlen > 0) {
        int to_process = pty->readlen + pty->remaininglen;
        int processed = vt100_screen_process_string(
            ((RunesTerm *)t)->scr, pty->readbuf, to_process);
        pty->remaininglen = to_process - processed;
        memmove(pty->readbuf, pty->readbuf + processed, pty->remaininglen);

        return 1;
    }
    else {
        runes_window_backend_request_close(t);

        return 0;
    }
}
