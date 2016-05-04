#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "runes.h"
#include "socket.h"

static int runes_socket_open_client(char *name);

int main (int argc, char *argv[])
{
    char *name, *buf;
    int s, i;
    size_t len, offset;
    uint32_t argc32 = argc, argv_len;

    name = runes_get_socket_name();
    s = runes_socket_open_client(name);

    len = sizeof(argc32) + sizeof(argv_len);
    for (i = 0; i < argc; ++i) {
        len += strlen(argv[i]) + 1;
    }

    buf = malloc(len);
    ((uint32_t*)buf)[0] = htonl(argc32);
    offset = sizeof(argc32) + sizeof(argv_len);
    ((uint32_t*)buf)[1] = htonl(len - offset);

    for (i = 0; i < argc; ++i) {
        size_t arg_len = strlen(argv[i]) + 1;
        memcpy(buf + offset, argv[i], arg_len);
        offset += arg_len;
    }

    send(s, buf, offset, 0);
    free(buf);
    shutdown(s, SHUT_RDWR);
}

static int runes_socket_open_client(char *name)
{
    int s;
    struct sockaddr_un client;

    if (strlen(name) + 1 > MAX_SOCKET_PATH_LEN) {
        runes_die("socket path %s is too long", name);
    }

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        runes_die("couldn't create socket: %s", strerror(errno));
    }

    client.sun_family = AF_UNIX;
    strcpy(client.sun_path, name);
    if (connect(s, (struct sockaddr*)(&client), sizeof(struct sockaddr_un))) {
        runes_die("couldn't connect to socket at %s: %s", name,
                  strerror(errno));
    }

    return s;
}
