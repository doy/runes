#include <stdlib.h>

#include "runes.h"

struct runes_loop_timer_data {
    RunesTerm *t;
    void (*cb)(RunesTerm*);
};

static void runes_loop_do_work(uv_work_t *req);
static void runes_loop_do_after_work(uv_work_t *req, int status);
static void runes_loop_timer_cb(uv_timer_t *handle);
static void runes_loop_free_handle(uv_handle_t *handle);

void runes_loop_init(RunesLoop *loop)
{
    loop->loop = uv_default_loop();
}

void runes_loop_init_term(RunesLoop *loop, RunesTerm *t)
{
    t->loop = loop;
    runes_window_backend_init_loop(t, loop);
    runes_pty_backend_init_loop(t, loop);
}

void runes_loop_run(RunesLoop *loop)
{
    uv_run(loop->loop, UV_RUN_DEFAULT);
}

void runes_loop_start_work(RunesLoop *loop, RunesTerm *t,
                           void (*work_cb)(RunesTerm*),
                           int (*after_work_cb)(RunesTerm*))
{
    void *data;

    data = malloc(sizeof(RunesLoopData));
    ((RunesLoopData *)data)->req.data = data;
    ((RunesLoopData *)data)->loop = loop;
    ((RunesLoopData *)data)->t = t;
    ((RunesLoopData *)data)->work_cb = work_cb;
    ((RunesLoopData *)data)->after_work_cb = after_work_cb;

    uv_queue_work(loop->loop, data, runes_loop_do_work,
                  runes_loop_do_after_work);
}

void runes_loop_timer_set(RunesLoop *loop, int timeout, int repeat,
                          RunesTerm *t, void (*cb)(RunesTerm*))
{
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

void runes_loop_cleanup(RunesLoop *loop)
{
    uv_loop_close(loop->loop);
}

static void runes_loop_do_work(uv_work_t *req)
{
    RunesLoopData *data = req->data;
    RunesTerm *t = data->t;

    data->work_cb(t);
}

static void runes_loop_do_after_work(uv_work_t *req, int status)
{
    RunesLoopData *data = req->data;
    RunesLoop *loop = data->loop;
    RunesTerm *t = data->t;
    int should_loop = 0;

    UNUSED(status);

    should_loop = data->after_work_cb(t);
    if (should_loop) {
        uv_queue_work(loop->loop, req, runes_loop_do_work,
                      runes_loop_do_after_work);
    }
    else {
        free(req);
    }
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
