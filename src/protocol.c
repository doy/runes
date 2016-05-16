#include <arpa/inet.h>
#include <errno.h>
#include <glib.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "runes.h"
#include "protocol.h"

#define RUNES_MAX_PACKET_SIZE (1024*1024)

static int runes_protocol_store_u32(GArray *buf, uint32_t i);
static int runes_protocol_store_strvec(GArray *buf, char *vec[]);
static int runes_protocol_store_str(GArray *buf, char *str);
static ssize_t runes_protocol_load_u32(char *buf, size_t len, uint32_t *i);
static ssize_t runes_protocol_load_strvec(char *buf, size_t len, char ***vec);
static ssize_t runes_protocol_load_str(char *buf, size_t len, char **str);
static int runes_protocol_read_bytes(int sock, size_t len, char *outbuf);
static int runes_protocol_write_bytes(int sock, char *buf, size_t len);

int runes_protocol_parse_new_term_message(
    char *buf, size_t len, struct runes_protocol_new_term_message *outmsg)
{
    uint32_t version;
    size_t offset = 0;

#define LOAD(type, var) \
    do { \
        ssize_t incr; \
        incr = runes_protocol_load_##type(buf + offset, len - offset, &var); \
        if (incr < 0) { \
            free(outmsg); \
            return 0; \
        } \
        offset += incr; \
    } while (0)

    LOAD(u32, version);
    if (version != RUNES_PROTOCOL_MESSAGE_VERSION) {
        /* backcompat code can go here eventually */
        runes_warn("unknown protocol version %d", version);
        free(outmsg);
        return 0;
    }

    LOAD(u32,    outmsg->argc);
    LOAD(strvec, outmsg->argv);
    LOAD(strvec, outmsg->envp);
    LOAD(str,    outmsg->cwd);

#undef LOAD

    return 1;
}

int runes_protocol_create_new_term_message(
    struct runes_protocol_new_term_message *msg,
    char **outbuf, size_t *outlen)
{
    GArray *buf;

    buf = g_array_new(FALSE, FALSE, 1);

#define STORE(type, val) \
    do { \
        if (!runes_protocol_store_##type(buf, val)) { \
            g_array_unref(buf); \
            return 0; \
        } \
    } while (0)

    STORE(u32, RUNES_PROTOCOL_MESSAGE_VERSION);

    STORE(u32,    msg->argc);
    STORE(strvec, msg->argv);
    STORE(strvec, msg->envp);
    STORE(str,    msg->cwd);

#undef STORE

    *outbuf = malloc(buf->len);
    memcpy(*outbuf, buf->data, buf->len);
    *outlen = buf->len;

    g_array_unref(buf);

    return 1;
}

void runes_protocol_free_new_term_message(
    struct runes_protocol_new_term_message *msg)
{
    char **p;

    p = msg->argv;
    while (*p) {
        free(*p);
        p++;
    }
    free(msg->argv);

    p = msg->envp;
    while (*p) {
        free(*p);
        p++;
    }
    free(msg->envp);

    free(msg->cwd);
}

int runes_protocol_read_packet(
    int sock, int *outtype, char **outbuf, size_t *outlen)
{
    uint32_t len, type;
    char *buf;

    if (!runes_protocol_read_bytes(sock, sizeof(len), (char *)&len)) {
        return 0;
    }
    len = ntohl(len);

    if (!runes_protocol_read_bytes(sock, sizeof(type), (char *)&type)) {
        return 0;
    }
    type = ntohl(type);

    if (len > RUNES_MAX_PACKET_SIZE) {
        runes_warn("packet of size %d is too big", len);
        return 0;
    }
    buf = malloc(len);

    if (!runes_protocol_read_bytes(sock, len, buf)) {
        free(buf);
        return 0;
    }

    *outlen = len;
    *outbuf = buf;
    *outtype = type;

    return 1;
}

int runes_protocol_send_packet(int sock, int type, char *buf, size_t len)
{
    uint32_t len32 = len, type32 = type;

    if (len > RUNES_MAX_PACKET_SIZE) {
        runes_warn("packet of size %d is too big", len);
        return 0;
    }

    if (type < 0 || type >= RUNES_PROTOCOL_NUM_MESSAGE_TYPES) {
        runes_warn("unknown message type %d", type);
        return 0;
    }

    len32 = htonl(len32);
    if (!runes_protocol_write_bytes(sock, (char *)&len32, sizeof(len32))) {
        runes_warn("failed to write packet to socket");
        return 0;
    }

    type32 = htonl(type32);
    if (!runes_protocol_write_bytes(sock, (char *)&type32, sizeof(type32))) {
        runes_warn("failed to write packet to socket");
        return 0;
    }

    if (!runes_protocol_write_bytes(sock, buf, len)) {
        runes_warn("failed to write packet to socket");
        return 0;
    }

    return 1;
}

static int runes_protocol_store_u32(GArray *buf, uint32_t i)
{
    uint32_t ni = htonl(i);

    g_array_append_vals(buf, (void *)&ni, sizeof(ni));

    return 1;
}

static int runes_protocol_store_strvec(GArray *buf, char *vec[])
{
    size_t len, i;

    for (len = 0; vec[len]; ++len);
    if (len > RUNES_MAX_PACKET_SIZE) {
        runes_warn("string vector of size %d is too big", len);
        return 0;
    }

#define STORE(type, val) \
    do { \
        if (!runes_protocol_store_##type(buf, val)) { \
            return 0; \
        } \
    } while (0)

    STORE(u32, len);
    for (i = 0; i < len; ++i) {
        STORE(str, vec[i]);
    }

#undef STORE

    return 1;
}

static int runes_protocol_store_str(GArray *buf, char *str)
{
    size_t len = strlen(str);

    if (len > RUNES_MAX_PACKET_SIZE) {
        runes_warn("string of size %d is too big", len);
        return 0;
    }

#define STORE(type, val) \
    do { \
        if (!runes_protocol_store_##type(buf, val)) { \
            return 0; \
        } \
    } while (0)

    STORE(u32, len);

    g_array_append_vals(buf, str, len);

#undef STORE

    return 1;
}

static ssize_t runes_protocol_load_u32(char *buf, size_t len, uint32_t *i)
{
    uint32_t hi;

    if (len < sizeof(uint32_t)) {
        return -1;
    }

    hi = ((uint32_t *)buf)[0];
    *i = ntohl(hi);

    return sizeof(uint32_t);
}

static ssize_t runes_protocol_load_strvec(char *buf, size_t len, char ***vec)
{
    ssize_t offset = 0;
    uint32_t veclen = 0;
    size_t i;

    *vec = NULL;

#define LOAD(type, var) \
    do { \
        ssize_t incr; \
        incr = runes_protocol_load_##type(buf + offset, len - offset, &var); \
        if (incr < 0) { \
            free(*vec); \
            for (i = 0; i < veclen; ++i) { \
                free(*vec[i]); \
            } \
            return -1; \
        } \
        offset += incr; \
    } while (0)

    LOAD(u32, veclen);
    if (veclen > len) {
        runes_warn("string of size %d is too big", strlen);
        return -1;
    }

    *vec = calloc(veclen + 1, sizeof(char *));

    for (i = 0; i < veclen; ++i) {
        LOAD(str, (*vec)[i]);
    }
    (*vec)[veclen] = NULL;

#undef LOAD

    return offset;
}

static ssize_t runes_protocol_load_str(char *buf, size_t len, char **str)
{
    ssize_t offset = 0;
    uint32_t strlen;

    *str = NULL;

#define LOAD(type, var) \
    do { \
        ssize_t incr; \
        incr = runes_protocol_load_##type(buf + offset, len - offset, &var); \
        if (incr < 0) { \
            free(*str); \
            return -1; \
        } \
        offset += incr; \
    } while (0)

    LOAD(u32, strlen);
    if (strlen > len) {
        runes_warn("string of size %d is too big", strlen);
        return -1;
    }

    *str = malloc(strlen + 1);

    memcpy(*str, buf + offset, strlen);
    (*str)[strlen] = '\0';
    offset += strlen;

#undef LOAD

    return offset;
}

static int runes_protocol_write_bytes(int sock, char *buf, size_t len)
{
    size_t bytes = 0;

    while (bytes < len) {
        ssize_t sent;

        sent = send(sock, buf + bytes, len - bytes, 0);
        if (sent <= 0 && errno != EINTR) {
            runes_warn(
                "couldn't write %d bytes to socket: %s",
                len, strerror(errno));
            return 0;
        }
        bytes += sent;
    }

    return 1;
}

static int runes_protocol_read_bytes(int sock, size_t len, char *outbuf)
{
    size_t bytes = 0;

    while (bytes < len) {
        ssize_t got;

        got = recv(sock, outbuf + bytes, len - bytes, 0);
        if (got <= 0 && errno != EINTR) {
            runes_warn(
                "couldn't read %d bytes from socket: %s",
                len, strerror(errno));
            return 0;
        }
        bytes += got;
    }

    return 1;
}
