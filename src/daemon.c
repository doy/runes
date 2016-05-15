#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "runes.h"
#include "daemon.h"

#include "loop.h"
#include "protocol.h"
#include "socket.h"
#include "term.h"

static int runes_daemon_handle_request(void *t);

RunesDaemon *runes_daemon_new(RunesLoop *loop, RunesWindowBackend *wb)
{
    RunesDaemon *daemon;

    daemon = calloc(1, sizeof(RunesDaemon));
    daemon->loop = loop;
    daemon->wb = wb;
    daemon->sock_name = runes_get_daemon_socket_name();
    daemon->sock = runes_socket_server_open(daemon->sock_name);
    runes_daemon_init_loop(daemon, loop);

    return daemon;
}

void runes_daemon_init_loop(RunesDaemon *daemon, RunesLoop *loop)
{
    runes_loop_start_work(
        loop, daemon->sock, daemon, runes_daemon_handle_request);
}

void runes_socket_delete(RunesDaemon *daemon)
{
    runes_socket_server_close(daemon->sock, daemon->sock_name);
    free(daemon->sock_name);

    free(daemon);
}

static int runes_daemon_handle_request(void *t)
{
    RunesDaemon *daemon = (RunesDaemon *)t;
    int client_sock;
    char *buf;
    size_t len;
    struct runes_protocol_message msg;
    RunesTerm *new_term;

    client_sock = runes_socket_server_accept(daemon->sock);

    if (!runes_protocol_read_packet(client_sock, &buf, &len)) {
        runes_warn("invalid packet received");
        return 1;
    }

    runes_socket_client_close(client_sock);

    if (!runes_protocol_parse_message(buf, len, &msg)) {
        runes_warn("couldn't parse message from client");
        free(buf);
        return 1;
    }

    new_term = runes_term_new(
        msg.argc, msg.argv, msg.envp, msg.cwd, daemon->wb);
    runes_term_register_with_loop(new_term, daemon->loop);

    free(buf);
    runes_protocol_free_message(&msg);

    return 1;
}
