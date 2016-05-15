#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "runes.h"
#include "socket.h"

#define MAX_SOCKET_PATH_LEN \
    (sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))

static void runes_socket_populate_sockaddr(
    char *path, struct sockaddr_un *addr);

int runes_socket_client_open(char *path)
{
    int s;
    struct sockaddr_un client;

    runes_socket_populate_sockaddr(path, &client);

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        runes_die("couldn't create socket: %s", strerror(errno));
    }

    if (connect(s, (struct sockaddr*)(&client), sizeof(struct sockaddr_un))) {
        runes_die(
            "couldn't connect to socket at %s: %s", path, strerror(errno));
    }

    return s;
}

int runes_socket_server_open(char *path)
{
    char *dir, *slash;
    int s;
    struct sockaddr_un server;

    runes_socket_populate_sockaddr(path, &server);

    dir = strdup(path);
    slash = strrchr(dir, '/');
    if (slash == NULL) {
        runes_die("socket path %s must be an absolute path", path);
    }
    *slash = '\0';
    runes_mkdir_p(dir);
    free(dir);

    unlink(path);

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        runes_die("couldn't create socket: %s", strerror(errno));
    }

    if (bind(s, (struct sockaddr*)(&server), sizeof(struct sockaddr_un))) {
        runes_die(
            "couldn't bind to socket %s: %s", path, strerror(errno));
    }

    if (chmod(path, S_IRUSR|S_IWUSR)) {
        runes_die(
            "couldn't chmod socket %s: %s", path, strerror(errno));
    }

    if (listen(s, 5)) {
        runes_die(
            "couldn't listen on socket %s: %s", path, strerror(errno));
    }

    return s;
}

int runes_socket_server_accept(int ss)
{
    struct sockaddr_un client;
    socklen_t len = sizeof(client);
    int cs;

    cs = accept(ss, (struct sockaddr*)(&client), &len);
    if (cs < 0) {
        runes_die("couldn't accept connection: %s", strerror(errno));
    }

    return cs;
}

void runes_socket_client_close(int s)
{
    close(s);
}

void runes_socket_server_close(int s, char *path)
{
    close(s);
    unlink(path);
}

static void runes_socket_populate_sockaddr(
    char *path, struct sockaddr_un *addr)
{
    size_t name_len = strlen(path) + 1; // including the nul byte

    if (name_len > MAX_SOCKET_PATH_LEN) {
        runes_die("socket path %s is too long", path);
    }

    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, path, name_len);
}
