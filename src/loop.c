#include <stdlib.h>

#include "runes.h"

struct runes_loop_timer_data {
    RunesTerm *t;
    void (*cb)(RunesTerm*);
};

static void runes_loop_timer_cb(uv_timer_t *handle);
static void runes_loop_free_handle(uv_handle_t *handle);

void runes_loop_init(RunesTerm *t)
{
    RunesLoop *loop = &t->loop;

    loop->loop = uv_default_loop();
    runes_window_backend_init_loop(t);
    runes_pty_backend_init_loop(t);
}

void runes_loop_run(RunesTerm *t)
{
    RunesLoop *loop = &t->loop;

    uv_run(loop->loop, UV_RUN_DEFAULT);
}

void runes_loop_timer_set(RunesTerm *t, int timeout, int repeat,
                          void (*cb)(RunesTerm*))
{
    RunesLoop *loop = &t->loop;
    uv_timer_t *timer_req;
    struct runes_loop_timer_data *timer_data;

    timer_req = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop->loop, timer_req);
    timer_data = malloc(sizeof(struct runes_loop_timer_data));
    timer_data->t = t;
    timer_data->cb = cb;
    timer_req->data = (void *)timer_data;
    uv_timer_start(timer_req, runes_loop_timer_cb, timeout, repeat);
}

void runes_loop_cleanup(RunesTerm *t)
{
    RunesLoop *loop = &t->loop;

    uv_loop_close(loop->loop);
}

static void runes_loop_timer_cb(uv_timer_t *handle)
{
    struct runes_loop_timer_data *data = handle->data;
    data->cb(data->t);
    uv_close(
        (uv_handle_t *)handle, runes_loop_free_handle);
}

static void runes_loop_free_handle(uv_handle_t *handle)
{
    free(handle->data);
    free(handle);
}
