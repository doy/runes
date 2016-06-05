#include <stdlib.h>
#include <sys/time.h>

#include "runes.h"
#include "loop.h"

struct runes_loop_data {
    struct event *event;
    void *t;
    int (*cb)(void*);
};

struct runes_loop_timer_data {
    struct event *event;
    struct timeval *timeout;
    void *t;
    void (*cb)(void*);
};

static void runes_loop_work_cb(evutil_socket_t fd, short what, void *arg);
static void runes_loop_timer_cb(evutil_socket_t fd, short what, void *arg);
static void runes_loop_free_timer_data(struct runes_loop_timer_data *data);

RunesLoop *runes_loop_new()
{
    RunesLoop *loop;

    loop = calloc(1, sizeof(RunesLoop));
    loop->base = event_base_new();

    return loop;
}

void runes_loop_run(RunesLoop *loop)
{
    event_base_dispatch(loop->base);
}

void runes_loop_stop(RunesLoop *loop)
{
    event_base_loopbreak(loop->base);
}

void runes_loop_start_work(
    RunesLoop *loop, int fd, void *t, int (*cb)(void*))
{
    struct runes_loop_data *data;

    data = malloc(sizeof(struct runes_loop_data));
    data->event = event_new(loop->base, fd, EV_READ, runes_loop_work_cb, data);
    data->t = t;
    data->cb = cb;

    event_add(data->event, NULL);
}

void *runes_loop_timer_set(
    RunesLoop *loop, int timeout, void *t, void (*cb)(void*))
{
    struct runes_loop_timer_data *data;

    data = malloc(sizeof(struct runes_loop_timer_data));
    data->event = event_new(loop->base, -1, 0, runes_loop_timer_cb, data);
    data->t = t;
    data->cb = cb;
    data->timeout = malloc(sizeof(struct timeval));
    data->timeout->tv_sec = 0;
    while (timeout >= 1000) {
        data->timeout->tv_sec += 1;
        timeout -= 1000;
    }
    data->timeout->tv_usec = timeout * 1000;

    event_add(data->event, data->timeout);

    return (void *)data;
}

void runes_loop_timer_clear(RunesLoop *loop, void *arg)
{
    struct runes_loop_timer_data *data = arg;

    UNUSED(loop);

    event_del(data->event);
    runes_loop_free_timer_data(data);
}

void runes_loop_delete(RunesLoop *loop)
{
    event_base_free(loop->base);

    free(loop);
}

static void runes_loop_work_cb(evutil_socket_t fd, short what, void *arg)
{
    struct runes_loop_data *data = arg;

    UNUSED(fd);
    UNUSED(what);

    if (data->cb(data->t)) {
        event_add(data->event, NULL);
    }
    else {
        event_free(data->event);
        free(data);
    }
}

static void runes_loop_timer_cb(evutil_socket_t fd, short what, void *arg)
{
    struct runes_loop_timer_data *data = arg;

    UNUSED(fd);
    UNUSED(what);

    data->cb(data->t);

    runes_loop_free_timer_data(data);
}

static void runes_loop_free_timer_data(struct runes_loop_timer_data *data)
{
    event_free(data->event);
    free(data->timeout);
    free(data);
}
