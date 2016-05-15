#ifndef _RUNES_SOCKET_H
#define _RUNES_SOCKET_H

int runes_socket_client_open(char *path);
int runes_socket_server_open(char *path);
int runes_socket_server_accept(int s);
void runes_socket_client_close(int s);
void runes_socket_server_close(int s, char *path);

#endif
