#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runes.h"

void runes_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    /* XXX make this do something else on windows */
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void runes_die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    /* XXX make this do something else on windows */
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(1);
}

char *runes_get_socket_name()
{
    char *home, *runtime_dir, *socket_dir, *socket_file;

    home = getenv("HOME");

    runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        sprintf_dup(&socket_dir, "%s/runes", runtime_dir);
    }
    else {
        sprintf_dup(&socket_dir, "%s/.runes", home);
    }

    sprintf_dup(&socket_file, "%s/runesd", socket_dir);
    free(socket_dir);

    return socket_file;
}

int sprintf_dup(char **out, const char *fmt, ...)
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

void mkdir_p(char *dir)
{
    char *path_component, *save_ptr, *tok_str, *partial_path;
    struct stat st;

    dir = strdup(dir);
    partial_path = strdup("/");
    tok_str = dir;
    while ((path_component = strtok_r(tok_str, "/", &save_ptr))) {
        char *new_path;

        sprintf_dup(&new_path, "%s%s/", partial_path, path_component);

        if (mkdir(new_path, 0755)) {
            if (errno != EEXIST) {
                runes_die("couldn't create directory %s: %s\n",
                          new_path, strerror(errno));
            }
        }

        free(partial_path);
        partial_path = new_path;
        tok_str = NULL;
    }

    if (stat(partial_path, &st)) {
        runes_die("couldn't stat %s: %s\n", partial_path, strerror(errno));
    }

    if (!S_ISDIR(st.st_mode)) {
        runes_die("couldn't create directory %s: %s\n",
                  partial_path, strerror(EEXIST));
    }

    free(partial_path);
    free(dir);
}
