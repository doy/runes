#include <stddef.h>
#include <stdlib.h>

#include "runes.h"

#include "protocol.h"
#include "socket.h"

extern char **environ;

int main (int argc, char *argv[])
{
    char *name, *buf;
    int s;
    size_t len;
    struct runes_protocol_message msg;

    msg.argc = argc;
    msg.argv = argv;
    msg.envp = environ;
    msg.cwd = runes_getcwd();

    name = runes_get_daemon_socket_name();
    s = runes_socket_client_open(name);
    if (!runes_protocol_create_message(&msg, &buf, &len)) {
        runes_warn("couldn't create message");
    }
    else {
        if (!runes_protocol_send_packet(s, buf, len)) {
            runes_warn("couldn't send packet");
            free(buf);
        }
    }

    free(buf);
    free(msg.cwd);
    runes_socket_client_close(s);

    return 0;
}
