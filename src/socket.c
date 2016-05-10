#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "runes.h"
#include "socket.h"

#include "loop.h"
#include "term.h"

static int runes_socket_open(RunesSocket *sock);
static void runes_socket_close(RunesSocket *sock);
static void runes_socket_accept(void *t);
static int runes_socket_handle_request(void *t);

RunesSocket *runes_socket_new(RunesLoop *loop, RunesWindowBackendGlobal *wg)
{
    RunesSocket *sock;

    sock = calloc(1, sizeof(RunesSocket));
    sock->loop = loop;
    sock->wg = wg;
    sock->name = runes_get_socket_name();
    sock->sock = runes_socket_open(sock);
    runes_socket_init_loop(sock, loop);

    return sock;
}

void runes_socket_init_loop(RunesSocket *sock, RunesLoop *loop)
{
    runes_loop_start_work(loop, sock, runes_socket_accept,
                          runes_socket_handle_request);
}

void runes_socket_delete(RunesSocket *sock)
{
    runes_socket_close(sock);
    free(sock->name);

    free(sock);
}

static int runes_socket_open(RunesSocket *sock)
{
    char *dir, *slash;
    int s;
    struct sockaddr_un server;

    if (strlen(sock->name) + 1 > MAX_SOCKET_PATH_LEN) {
        runes_die("socket path %s is too long", sock->name);
    }

    dir = strdup(sock->name);
    slash = strrchr(dir, '/');
    if (slash == NULL) {
        runes_die("socket path %s must be an absolute path", sock->name);
    }
    *slash = '\0';
    runes_mkdir_p(dir);
    free(dir);

    unlink(sock->name);

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        runes_die("couldn't create socket: %s", strerror(errno));
    }

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, sock->name);
    if (bind(s, (struct sockaddr*)(&server), sizeof(struct sockaddr_un))) {
        runes_die("couldn't bind to socket %s: %s", sock->name,
                  strerror(errno));
    }

    if (chmod(sock->name, S_IRUSR|S_IWUSR)) {
        runes_die("couldn't chmod socket %s: %s", sock->name,
                  strerror(errno));
    }

    if (listen(s, 5)) {
        runes_die("couldn't listen on socket %s: %s", sock->name,
                  strerror(errno));
    }

    return s;
}

static void runes_socket_close(RunesSocket *sock)
{
    close(sock->sock);
    unlink(sock->name);
}

static void runes_socket_accept(void *t)
{
    RunesSocket *sock = (RunesSocket *)t;
    struct sockaddr_un client;
    socklen_t len = sizeof(client);

    sock->client_sock = accept(sock->sock, (struct sockaddr*)(&client), &len);
}

static int runes_socket_handle_request(void *t)
{
    RunesSocket *sock = (RunesSocket *)t;
    ssize_t bytes;
    uint32_t argc, argv_len;
    char **argv, *argv_buf;

    if (sock->client_sock < 0) {
        runes_die("couldn't accept connection: %s", strerror(errno));
    }

    bytes = recv(sock->client_sock, (void*)(&argc), sizeof(argc), 0);
    if (bytes < (int)sizeof(argc)) {
        runes_warn("invalid message received: got %d bytes, expected 4",
                   bytes);
        close(sock->client_sock);
        return 1;
    }

    argc = ntohl(argc);
    if (argc > 1024) {
        runes_warn("invalid message received: argc = %d, must be < 1024",
                   argc);
        close(sock->client_sock);
        return 1;
    }
    argv = malloc(argc * sizeof(char*));

    bytes = recv(sock->client_sock, (void*)(&argv_len), sizeof(argv_len), 0);
    if (bytes < (int)sizeof(argc)) {
        runes_warn("invalid message received: got %d bytes, expected 4",
                   bytes);
        close(sock->client_sock);
        free(argv);
        return 1;
    }

    argv_len = ntohl(argv_len);
    if (argv_len > 131072) {
        runes_warn("invalid message received: argv_len = %d, must be < %d",
                   argv_len, 131072);
        close(sock->client_sock);
        free(argv);
        return 1;
    }
    argv_buf = malloc(argv_len + 1);

    bytes = recv(sock->client_sock, argv_buf, argv_len, 0);
    if (bytes < argv_len) {
        runes_warn("invalid message received: got %d bytes, expected %d",
                   bytes, argv_len);
        close(sock->client_sock);
        free(argv);
        free(argv_buf);
        return 1;
    }

    close(sock->client_sock);

    if (argc > 0) {
        size_t offset = 0;
        int i;

        for (i = 0; i < (int)argc; ++i) {
            char *next_null;

            if (offset >= argv_len) {
                runes_die("args in argv_buf don't match argc of %d", argc);
            }
            argv[i] = argv_buf + offset;
            next_null = memchr(argv_buf + offset, '\0', argv_len - offset);
            if (!next_null) {
                runes_die("args in argv_buf don't match argc of %d", argc);
            }
            offset = next_null - argv_buf + 1;
        }

        runes_term_register_with_loop(
            runes_term_new(argc, argv, sock->wg), sock->loop);
    }

    free(argv);
    free(argv_buf);

    return 1;
}
