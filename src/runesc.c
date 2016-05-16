#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "runes.h"

#include "protocol.h"
#include "socket.h"

extern char **environ;

static void runes_client_send_kill_daemon_message(int s);
static void runes_client_send_new_term_message(int s, int argc, char *argv[]);

int main (int argc, char *argv[])
{
    char *name;
    int s;

    name = runes_get_daemon_socket_name();
    s = runes_socket_client_open(name);

    if (argc > 1 && !strcmp(argv[1], "--kill-daemon")) {
        runes_client_send_kill_daemon_message(s);
    }
    else {
        runes_client_send_new_term_message(s, argc, argv);
    }

    runes_socket_client_close(s);

#ifdef RUNES_VALGRIND
    free(name);
#endif

    return 0;
}

static void runes_client_send_kill_daemon_message(int s)
{
    if (!runes_protocol_send_packet(s, RUNES_PROTOCOL_KILL_DAEMON, "", 0)) {
        runes_warn("couldn't send packet");
    }
}

static void runes_client_send_new_term_message(int s, int argc, char *argv[])
{
    struct runes_protocol_new_term_message msg;
    char *buf;
    size_t len;

    msg.argc = argc;
    msg.argv = argv;
    msg.envp = environ;
    msg.cwd = runes_getcwd();

    if (runes_protocol_create_new_term_message(&msg, &buf, &len)) {
        if (!runes_protocol_send_packet(s, RUNES_PROTOCOL_NEW_TERM, buf, len)) {
            runes_warn("couldn't send packet");
        }
    }
    else {
        runes_warn("couldn't create message");
    }

#ifdef RUNES_VALGRIND
    free(buf);
    free(msg.cwd);
#endif
}
