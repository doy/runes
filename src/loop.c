#include <stdlib.h>

#include "runes.h"
#include "loop.h"

struct runes_loop_data {
    uv_work_t req;
    RunesLoop *loop;
    void *t;
    void (*work_cb)(void*);
    int (*after_work_cb)(void*);
};

struct runes_loop_timer_data {
    void *t;
    void (*cb)(void*);
};

static void runes_loop_do_work(uv_work_t *req);
static void runes_loop_do_after_work(uv_work_t *req, int status);
static void runes_loop_timer_cb(uv_timer_t *handle);
static void runes_loop_free_handle(uv_handle_t *handle);

RunesLoop *runes_loop_new()
{
    RunesLoop *loop;

    loop = calloc(1, sizeof(RunesLoop));
    loop->loop = uv_default_loop();

    return loop;
}

void runes_loop_run(RunesLoop *loop)
{
    uv_run(loop->loop, UV_RUN_DEFAULT);
}

void runes_loop_start_work(RunesLoop *loop, void *t,
                           void (*work_cb)(void*),
                           int (*after_work_cb)(void*))
{
    struct runes_loop_data *data;

    data = malloc(sizeof(struct runes_loop_data));
    data->req.data = data;
    data->loop = loop;
    data->t = t;
    data->work_cb = work_cb;
    data->after_work_cb = after_work_cb;

    uv_queue_work(loop->loop, (void*)data, runes_loop_do_work,
                  runes_loop_do_after_work);
}

void runes_loop_timer_set(RunesLoop *loop, int timeout, int repeat,
                          void *t, void (*cb)(void*))
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

void runes_loop_delete(RunesLoop *loop)
{
    uv_loop_close(loop->loop);

    free(loop);
}

static void runes_loop_do_work(uv_work_t *req)
{
    struct runes_loop_data *data = req->data;
    void *t = data->t;

    data->work_cb(t);
}

static void runes_loop_do_after_work(uv_work_t *req, int status)
{
    struct runes_loop_data *data = req->data;
    RunesLoop *loop = data->loop;
    void *t = data->t;
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
