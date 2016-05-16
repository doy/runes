#ifndef _RUNES_PROTOCOL_H
#define _RUNES_PROTOCOL_H

#include <stdint.h>

#define RUNES_PROTOCOL_MESSAGE_VERSION 1

struct runes_protocol_new_term_message {
    uint32_t argc;
    char **argv;
    char **envp;
    char *cwd;
};

enum runes_protocol_message_type {
    RUNES_PROTOCOL_NEW_TERM,
    RUNES_PROTOCOL_NUM_MESSAGE_TYPES
};

int runes_protocol_parse_new_term_message(
    char *buf, size_t len, struct runes_protocol_new_term_message *outmsg);
int runes_protocol_create_new_term_message(
    struct runes_protocol_new_term_message *msg,
    char **outbuf, size_t *outlen);
void runes_protocol_free_new_term_message(
    struct runes_protocol_new_term_message *msg);
int runes_protocol_read_packet(
    int sock, int *outtype, char **outbuf, size_t *outlen);
int runes_protocol_send_packet(int s, int type, char *buf, size_t len);

#endif
