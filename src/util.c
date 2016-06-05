#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "runes.h"

static void runes_vwarn(const char *fmt, va_list ap);

void runes_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    runes_vwarn(fmt, ap);
    va_end(ap);
}

void runes_die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    runes_vwarn(fmt, ap);
    va_end(ap);

    exit(1);
}

char *runes_get_daemon_socket_name()
{
    char *home, *runtime_dir, *socket_dir, *socket_file;

    home = getenv("HOME");

    runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        runes_sprintf_dup(&socket_dir, "%s/"RUNES_PROGRAM_NAME, runtime_dir);
    }
    else {
        runes_sprintf_dup(&socket_dir, "%s/."RUNES_PROGRAM_NAME, home);
    }

    runes_sprintf_dup(&socket_file, "%s/"RUNES_PROGRAM_NAME"d", socket_dir);
    free(socket_dir);

    return socket_file;
}

int runes_sprintf_dup(char **out, const char *fmt, ...)
{
    int outlen = 0;
    va_list ap;

    va_start(ap, fmt);
    outlen = vsnprintf(*out, outlen, fmt, ap);
    va_end(ap);

    *out = malloc(outlen + 1);

    va_start(ap, fmt);
    outlen = vsnprintf(*out, outlen + 1, fmt, ap);
    va_end(ap);

    return outlen;
}

void runes_mkdir_p(char *dir)
{
    char *path_component, *save_ptr, *tok_str, *partial_path;
    struct stat st;

    dir = strdup(dir);
    partial_path = strdup("/");
    tok_str = dir;
    while ((path_component = strtok_r(tok_str, "/", &save_ptr))) {
        char *new_path;

        runes_sprintf_dup(&new_path, "%s%s/", partial_path, path_component);

        if (mkdir(new_path, 0755)) {
            if (errno != EEXIST) {
                runes_die("couldn't create directory %s: %s",
                          new_path, strerror(errno));
            }
        }

        free(partial_path);
        partial_path = new_path;
        tok_str = NULL;
    }

    if (stat(partial_path, &st)) {
        runes_die("couldn't stat %s: %s", partial_path, strerror(errno));
    }

    if (!S_ISDIR(st.st_mode)) {
        runes_die("couldn't create directory %s: %s",
                  partial_path, strerror(EEXIST));
    }

    free(partial_path);
    free(dir);
}

char *runes_getcwd(void)
{
    char buf[4096];

    if (getcwd(buf, 4096)) {
        return strdup(buf);
    }
    else {
        runes_die("getcwd: %s", strerror(errno));
        return NULL;
    }
}

static void runes_vwarn(const char *fmt, va_list ap)
{
    size_t fmt_len;
    char *fmt_with_nl;

    fmt_len = strlen(fmt);
    fmt_with_nl = malloc(fmt_len + 2);
    strcpy(fmt_with_nl, fmt);
    fmt_with_nl[fmt_len] = '\n';
    fmt_with_nl[fmt_len + 1] = '\0';

    /* XXX make this do something else on windows */
    vfprintf(stderr, fmt_with_nl, ap);
}
