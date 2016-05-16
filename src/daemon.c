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
static void runes_daemon_handle_new_term_message(
    RunesDaemon *daemon, char *buf, size_t len);

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
    int client_sock, type, ret = 1;
    char *buf;
    size_t len;

    client_sock = runes_socket_server_accept(daemon->sock);

    if (runes_protocol_read_packet(client_sock, &type, &buf, &len)) {
        switch (type) {
        case RUNES_PROTOCOL_NEW_TERM:
            runes_daemon_handle_new_term_message(daemon, buf, len);
            break;
        case RUNES_PROTOCOL_KILL_DAEMON:
            ret = 0;
            runes_loop_stop(daemon->loop);
            break;
        default:
            runes_warn("unknown packet type: %d", type);
            break;
        }

        free(buf);
    }
    else {
        runes_warn("invalid packet received");
    }

    runes_socket_client_close(client_sock);

    return ret;
}

static void runes_daemon_handle_new_term_message(
    RunesDaemon *daemon, char *buf, size_t len)
{
    struct runes_protocol_new_term_message msg;

    if (runes_protocol_parse_new_term_message(buf, len, &msg)) {
        RunesTerm *new_term;

        new_term = runes_term_new(
            msg.argc, msg.argv, msg.envp, msg.cwd, daemon->wb);
        runes_term_register_with_loop(new_term, daemon->loop);

        runes_protocol_free_new_term_message(&msg);
    }
    else {
        runes_warn("couldn't parse message from client");
    }
}
