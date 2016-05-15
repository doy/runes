#ifndef _RUNES_PROTOCOL_H
#define _RUNES_PROTOCOL_H

#include <stdint.h>

#define RUNES_PROTOCOL_MESSAGE_VERSION 1

struct runes_protocol_message {
    uint32_t argc;
    char **argv;
    char **envp;
    char *cwd;
};

int runes_protocol_parse_message(
    char *buf, size_t len, struct runes_protocol_message *outmsg);
int runes_protocol_create_message(
    struct runes_protocol_message *msg, char **outbuf, size_t *outlen);
void runes_protocol_free_message(struct runes_protocol_message *msg);
int runes_protocol_read_packet(int sock, char **outbuf, size_t *outlen);
int runes_protocol_send_packet(int s, char *buf, size_t len);

#endif
