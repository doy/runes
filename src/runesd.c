#include <locale.h>

#include "runes.h"
#include "loop.h"
#include "socket.h"

int main (int argc, char *argv[])
{
    RunesLoop loop;
    RunesSocket socket;

    UNUSED(argv);

    if (argc > 1) {
        runes_die("runesd takes no arguments; pass them to runesc instead.");
    }

    setlocale(LC_ALL, "");

    runes_loop_init(&loop);
    runes_socket_init(&socket, &loop);

    runes_loop_run(&loop);

    runes_socket_cleanup(&socket);
    runes_loop_cleanup(&loop);

    return 0;
}
