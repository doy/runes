#ifndef _RUNES_PTY_H
#define _RUNES_PTY_H

#define RUNES_READ_BUFFER_LENGTH 4096

struct runes_pty {
    int master;
    int slave;
    pid_t child_pid;

    char readbuf[RUNES_READ_BUFFER_LENGTH];
    int readlen;
    int remaininglen;

    int scheduled_flush: 1;
};

RunesPty *runes_pty_new(void);
void runes_pty_spawn_subprocess(RunesTerm *t, char *envp[], char *cwd);
void runes_pty_init_loop(RunesTerm *t, RunesLoop *loop);
void runes_pty_set_window_size(
    RunesTerm *t, int row, int col, int xpixel, int ypixel);
void runes_pty_write(RunesTerm *t, char *buf, size_t len);
void runes_pty_request_close(RunesTerm *t);
void runes_pty_delete(RunesPty *pty);

#endif
