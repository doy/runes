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
#include "term.h"

static int runes_daemon_open_socket(char *sock_name);
static void runes_daemon_close_socket(RunesDaemon *sock);
static int runes_daemon_handle_request(void *t);

RunesDaemon *runes_daemon_new(RunesLoop *loop, RunesWindowBackend *wb)
{
    RunesDaemon *daemon;

    daemon = calloc(1, sizeof(RunesDaemon));
    daemon->loop = loop;
    daemon->wb = wb;
    daemon->sock_name = runes_get_daemon_socket_name();
    daemon->sock = runes_daemon_open_socket(daemon->sock_name);
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
    runes_daemon_close_socket(daemon);
    free(daemon->sock_name);

    free(daemon);
}

static int runes_daemon_open_socket(char *sock_name)
{
    char *dir, *slash;
    int s;
    struct sockaddr_un server;

    if (strlen(sock_name) + 1 > MAX_SOCKET_PATH_LEN) {
        runes_die("socket path %s is too long", sock_name);
    }

    dir = strdup(sock_name);
    slash = strrchr(dir, '/');
    if (slash == NULL) {
        runes_die("socket path %s must be an absolute path", sock_name);
    }
    *slash = '\0';
    runes_mkdir_p(dir);
    free(dir);

    unlink(sock_name);

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        runes_die("couldn't create socket: %s", strerror(errno));
    }

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, sock_name);
    if (bind(s, (struct sockaddr*)(&server), sizeof(struct sockaddr_un))) {
        runes_die("couldn't bind to socket %s: %s", sock_name,
                  strerror(errno));
    }

    if (chmod(sock_name, S_IRUSR|S_IWUSR)) {
        runes_die("couldn't chmod socket %s: %s", sock_name,
                  strerror(errno));
    }

    if (listen(s, 5)) {
        runes_die("couldn't listen on socket %s: %s", sock_name,
                  strerror(errno));
    }

    return s;
}

static void runes_daemon_close_socket(RunesDaemon *daemon)
{
    close(daemon->sock);
    unlink(daemon->sock_name);
}

static int runes_daemon_handle_request(void *t)
{
    RunesDaemon *daemon = (RunesDaemon *)t;
    struct sockaddr_un client;
    socklen_t len = sizeof(client);
    ssize_t bytes;
    uint32_t argc, argv_len;
    char **argv, *argv_buf;

    daemon->client_sock = accept(
        daemon->sock, (struct sockaddr*)(&client), &len);
    if (daemon->client_sock < 0) {
        runes_die("couldn't accept connection: %s", strerror(errno));
    }

    bytes = recv(daemon->client_sock, (void*)(&argc), sizeof(argc), 0);
    if (bytes < (int)sizeof(argc)) {
        runes_warn("invalid message received: got %d bytes, expected 4",
                   bytes);
        close(daemon->client_sock);
        return 1;
    }

    argc = ntohl(argc);
    if (argc > 1024) {
        runes_warn("invalid message received: argc = %d, must be < 1024",
                   argc);
        close(daemon->client_sock);
        return 1;
    }
    argv = malloc(argc * sizeof(char*));

    bytes = recv(daemon->client_sock, (void*)(&argv_len), sizeof(argv_len), 0);
    if (bytes < (int)sizeof(argc)) {
        runes_warn("invalid message received: got %d bytes, expected 4",
                   bytes);
        close(daemon->client_sock);
        free(argv);
        return 1;
    }

    argv_len = ntohl(argv_len);
    if (argv_len > 131072) {
        runes_warn("invalid message received: argv_len = %d, must be < %d",
                   argv_len, 131072);
        close(daemon->client_sock);
        free(argv);
        return 1;
    }
    argv_buf = malloc(argv_len + 1);

    bytes = recv(daemon->client_sock, argv_buf, argv_len, 0);
    if (bytes < argv_len) {
        runes_warn("invalid message received: got %d bytes, expected %d",
                   bytes, argv_len);
        close(daemon->client_sock);
        free(argv);
        free(argv_buf);
        return 1;
    }

    close(daemon->client_sock);

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
            runes_term_new(argc, argv, daemon->wb), daemon->loop);
    }

    free(argv);
    free(argv_buf);

    return 1;
}
