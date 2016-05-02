#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "runes.h"

static void runes_pty_backend_read(RunesTerm *t);
static int runes_pty_backend_got_data(RunesTerm *t);

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

        cmd = t->config.cmd;
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

void runes_pty_backend_init_loop(RunesTerm *t)
{
    runes_loop_start_work(t, runes_pty_backend_read,
                          runes_pty_backend_got_data);
}

void runes_pty_backend_set_window_size(RunesTerm *t)
{
    struct winsize size;

    size.ws_row = t->scr.grid->max.row;
    size.ws_col = t->scr.grid->max.col;
    size.ws_xpixel = t->display.xpixel;
    size.ws_ypixel = t->display.ypixel;
    ioctl(t->pty.master, TIOCSWINSZ, &size);
}

void runes_pty_backend_write(RunesTerm *t, char *buf, size_t len)
{
    write(t->pty.master, buf, len);
    if (t->display.row_visible_offset != 0) {
        t->display.row_visible_offset = 0;
        t->scr.dirty = 1;
        runes_window_backend_request_flush(t);
    }
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

static void runes_pty_backend_read(RunesTerm *t)
{
    RunesPtyBackend *pty = &t->pty;

    runes_window_backend_request_flush(t);
    pty->readlen = read(
        pty->master, pty->readbuf + pty->remaininglen,
        RUNES_READ_BUFFER_LENGTH - pty->remaininglen);
}

static int runes_pty_backend_got_data(RunesTerm *t)
{
    RunesPtyBackend *pty = &t->pty;

    if (pty->readlen > 0) {
        int to_process = pty->readlen + pty->remaininglen;
        int processed = vt100_screen_process_string(
            &t->scr, pty->readbuf, to_process);
        pty->remaininglen = to_process - processed;
        memmove(pty->readbuf, pty->readbuf + processed, pty->remaininglen);

        return 1;
    }
    else {
        runes_window_backend_request_close(t);

        return 0;
    }
}
