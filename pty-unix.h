#ifndef _RUNES_PTY_H
#define _RUNES_PTY_H

struct runes_pty {
    int master;
    int slave;
    pid_t child_pid;
};

void runes_pty_backend_spawn_subprocess(RunesTerm *t);
void runes_pty_backend_start_loop(RunesTerm *t);
void runes_pty_backend_set_window_size(RunesTerm *t);
void runes_pty_backend_write(RunesTerm *t, char *buf, size_t len);
void runes_pty_backend_request_close(RunesTerm *t);
void runes_pty_backend_cleanup(RunesTerm *t);

#endif
