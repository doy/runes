#ifndef _RUNES_SOCKET_H
#define _RUNES_SOCKET_H

#include <sys/un.h>

struct runes_socket {
    RunesLoop *loop;
    char *name;
    int sock;
    int client_sock;
};

void runes_socket_init(RunesSocket *sock, RunesLoop *loop);
void runes_socket_init_loop(RunesSocket *sock, RunesLoop *loop);
void runes_socket_send_client_message(int argc, char **argv);
void runes_socket_cleanup(RunesSocket *sock);

#define MAX_SOCKET_PATH_LEN \
    (sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))

#endif
