#ifndef _RUNES_DAEMON_H
#define _RUNES_DAEMON_H

#include <sys/un.h>

struct runes_daemon {
    RunesLoop *loop;
    RunesWindowBackend *wb;
    char *sock_name;
    int sock;
    int client_sock;
};

RunesDaemon *runes_daemon_new(RunesLoop *loop, RunesWindowBackend *wb);
void runes_daemon_init_loop(RunesDaemon *daemon, RunesLoop *loop);
void runes_daemon_send_client_message(int argc, char **argv);
void runes_daemon_delete(RunesDaemon *daemon);

#define MAX_SOCKET_PATH_LEN \
    (sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))

#endif
