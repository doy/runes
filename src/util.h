#ifndef _RUNES_UTIL_H
#define _RUNES_UTIL_H

#define UNUSED(x) ((void)x)

void runes_warn(const char *fmt, ...);
void runes_die(const char *fmt, ...);
char *runes_get_socket_name();
int sprintf_dup(char **out, const char *fmt, ...);
void mkdir_p(char *dir);

#endif
