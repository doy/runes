#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runes.h"

static void runes_config_set_defaults(RunesTerm *t);
static FILE *runes_config_get_config_file();
static void runes_config_process_config_file(RunesTerm *t, FILE *config_file);
static void runes_config_set(RunesTerm *t, char *key, char *value);

void runes_config_init(RunesTerm *t, int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    memset((void *)t, 0, sizeof(*t));

    runes_config_set_defaults(t);
    runes_config_process_config_file(t, runes_config_get_config_file());
}

static void runes_config_set_defaults(RunesTerm *t)
{
    t->font_name = "monospace 10";
}

static FILE *runes_config_get_config_file()
{
    char *home, *config_dir, *path;
    size_t home_len, config_dir_len;
    FILE *file;

    home = getenv("HOME");
    home_len = strlen(home);

    config_dir = getenv("XDG_CONFIG_HOME");
    if (config_dir) {
        config_dir = strdup(config_dir);
    }
    else {
        config_dir = malloc(home_len + sizeof("/.config") + 1);
        strcpy(config_dir, home);
        strcpy(config_dir + home_len, "/.config");
    }
    config_dir_len = strlen(config_dir);

    path = malloc(config_dir_len + sizeof("/runes/runes.conf") + 1);
    strcpy(path, config_dir);
    strcpy(path + config_dir_len, "/runes/runes.conf");
    free(config_dir);

    if ((file = fopen(path, "r"))) {
        free(path);
        return file;
    }

    free(path);
    path = malloc(home_len + sizeof("/.runesrc") + 1);
    strcpy(path, home);
    strcpy(path + home_len, "/.runesrc");

    if ((file = fopen(path, "r"))) {
        free(path);
        return file;
    }

    free(path);

    if ((file = fopen("/etc/runesrc", "r"))) {
        return file;
    }

    return NULL;
}

static void runes_config_process_config_file(RunesTerm *t, FILE *config_file)
{
    char line[1024];

    if (!config_file) {
        return;
    }

    while (fgets(line, 1024, config_file)) {
        char *kbegin, *kend, *vbegin, *vend;
        size_t len;

        len = strlen(line);
        if (line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        kbegin = line + strspn(line, " \t");
        kend = kbegin + strcspn(kbegin, " \t=");
        vbegin = kend + strspn(kend, " \t");
        if (*vbegin != '=') {
            fprintf(stderr, "couldn't parse line: '%s'\n", line);
        }
        vbegin++;
        vbegin = vbegin + strspn(vbegin, " \t");
        vend = line + len;

        *kend = '\0';
        *vend = '\0';

        runes_config_set(t, kbegin, vbegin);
    }
}

static void runes_config_set(RunesTerm *t, char *key, char *val)
{
    if (!strcmp(key, "font")) {
        t->font_name = strdup(val);
    }
    else {
        fprintf(stderr, "unknown option: '%s'\n", key);
    }
}
