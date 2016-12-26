/*
 * Copyright 2017 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <memory.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "mpp_event_trigger.h"

#include "mpp_log.h"
#include "rk_type.h"

struct event_ctx_impl {
    int (*notify)(void *param);

    event_trigger trigger;

    pthread_cond_t condition;
    pthread_mutex_t mutex;
    RK_U32 semval;

    pthread_t thr;
    RK_S32 flag;

    struct event_packet *ea;
    int event_idx;

    void *parent;
};

static int event_create(struct event_ctx_impl *ctx)
{
    int ret;

    ret = pthread_cond_init(&ctx->condition, NULL);
    if (ret != 0)
        return -1;

    ret = pthread_mutex_init(&ctx->mutex, NULL);
    if (ret != 0)
        return -1;

    ctx->semval = ctx->ea->e[0].idx;
    mpp_log_f("with %u\n", ctx->semval);

    return 0;
}

static void event_destroy(struct event_ctx_impl *ctx)
{
    pthread_cond_destroy(&ctx->condition);
    pthread_mutex_destroy(&ctx->mutex);
}

/* initialize and schedule next event */
static void event_init(struct event_ctx_impl *ctx)
{
    struct ievent *e_curr;
    struct ievent *e_next;

    pthread_mutex_lock(&ctx->mutex);
    e_curr = &ctx->ea->e[ctx->event_idx % ctx->ea->cnt];
    e_next = &ctx->ea->e[(++ctx->event_idx) % ctx->ea->cnt];

    mpp_log("curr %d, next %d\n",
            e_curr->idx, e_next->idx);

    if (e_next->idx > e_curr->idx)
        ctx->semval = e_next->idx - e_curr->idx;
    else if (ctx->ea->loop > 0)
        ctx->semval = e_next->idx + ctx->ea->loop - e_curr->idx;
    else
        ctx->flag = 0;

    mpp_log_f("semval %u\n", ctx->semval);
    pthread_mutex_unlock(&ctx->mutex);
}

/* wait the event trigger deadline */
/*static void event_wait(struct event_ctx_impl *ctx)
{
    pthread_mutex_lock(&ctx->mutex);
    while (ctx->semval > 0)
        pthread_cond_wait(&ctx->condition, &ctx->mutex);

    ctx->semval--;
    pthread_mutex_unlock(&ctx->mutex);
}*/

/* wait the event trigger deadline with timeout, avoid dead lock */
static int event_timed_wait(struct event_ctx_impl *ctx, unsigned int milli_sec)
{
    int err = 0;
    struct timespec final_time;
    struct timeval curr_time;
    long int microdelay;

    gettimeofday(&curr_time, NULL);
    /*
     * convert timeval to timespec and add delay in milliseconds
     * for the timeout
     */
    microdelay = ((milli_sec * 1000 + curr_time.tv_usec));
    final_time.tv_sec = curr_time.tv_sec + (microdelay / 1000000);
    final_time.tv_nsec = (microdelay % 1000000) * 1000;
    pthread_mutex_lock(&ctx->mutex);
    while (ctx->semval > 0) {
        err = pthread_cond_timedwait(&ctx->condition, &ctx->mutex, &final_time);
        if (err != 0)
            ctx->semval--;
    }
    ctx->semval--;
    pthread_mutex_unlock(&ctx->mutex);
    return err;
}

/* event heart beat */
static void event_down(struct event_ctx_impl *ctx)
{
    pthread_mutex_lock(&ctx->mutex);
    ctx->semval--;
    pthread_cond_signal(&ctx->condition);
    pthread_mutex_unlock(&ctx->mutex);
}

/* a callback to notify the event trigger that there is a event heart beat */
static int event_notify(void *param)
{
    struct event_ctx_impl *ctx = (struct event_ctx_impl *)param;

    if (ctx->flag)
        event_down(ctx);

    return 0;
}

static void *event_trigger_thread(void *param)
{
    struct event_ctx_impl *ctx = (struct event_ctx_impl *)param;
    struct timeval curr_time;
    long start_time, curr;
    RK_S32 ret;

    gettimeofday(&curr_time, NULL);

    start_time = curr_time.tv_sec * 1000 + curr_time.tv_usec / 1000;
    event_create(ctx);

    while (1) {
        ret = event_timed_wait(ctx, ctx->semval * 10000);
        if (!ctx->flag)
            break;

        if (ret == ETIMEDOUT) {
            mpp_err("wait event timeout\n");
            break;
        }

        gettimeofday(&curr_time, NULL);

        curr = curr_time.tv_sec * 1000 + curr_time.tv_usec / 1000;
        /* TODO, trigger event */
        mpp_log("[%ld ms] event idx %d triggered\n",
                curr - start_time, ctx->event_idx);
        mpp_log("cnt %d\n", ctx->ea->cnt);
        ctx->trigger(ctx->parent, ctx->ea->e[ctx->event_idx % ctx->ea->cnt].event);

        /* schedule next event */
        event_init(ctx);
        if (!ctx->flag)
            break;
    }

    event_destroy(ctx);

    mpp_log_f("exit\n");

    return NULL;
}

struct event_ctx* event_ctx_create(struct event_packet *ea,
                                   event_trigger trigger, void *parent)
{
    struct event_ctx_impl *ctx = (struct event_ctx_impl *)malloc(sizeof(*ctx));

    if (ctx == NULL) {
        mpp_err("allocate event ctx failed\n");
        return NULL;
    }

    assert(ea->cnt <= 128);

    ctx->event_idx = 0;
    ctx->ea = ea;

    ctx->notify = event_notify;
    ctx->trigger = trigger;
    ctx->parent = parent;

    ctx->flag = 1;
    pthread_create(&ctx->thr, NULL, event_trigger_thread, ctx);

    return (struct event_ctx *)ctx;
}

void event_ctx_release(struct event_ctx *ctx)
{
    void *ret;
    struct event_ctx_impl *ictx = (struct event_ctx_impl *)ctx;

    assert(ctx != NULL);

    if (ictx->flag) {
        ictx->flag = 0;
        ictx->semval = 0;
        pthread_cond_signal(&ictx->condition);
    }

    pthread_join(ictx->thr, &ret);

    free(ictx);
}

#ifdef EVENT_TRIGGER_TEST
/*
 * the following codes is the sample to use the event trigger
 */
struct event_impl {
    int cnt;
    int loop;
    int idx[128];
    int event[128];
};

/* a callback to notify test that a event occur */
static void event_occur(void *parent, void *event)
{
    int *e = (int *)event;
    mpp_log("event %d occur\n", *e);
}

int main(int argc, char **argv)
{
    struct event_ctx *ctx;
    int i = 0;
    struct event_impl ie;
    struct event_packet ea;

    ie.cnt = 4;
    ie.loop = 25;
    ie.event[0] = ie.idx[0] = 4;
    ie.event[1] = ie.idx[1] = 8;
    ie.event[2] = ie.idx[2] = 18;
    ie.event[3] = ie.idx[3] = 20;

    ea.cnt = ie.cnt;
    ea.loop = ie.loop;

    for (i = 0; i < ie.cnt; ++i) {
        ea.e[i].idx = ie.idx[i];
        ea.e[i].event = &ie.event[i];
    }

    ctx = event_ctx_create(&ea, event_occur, NULL);

    while (i++ < 100) {
        usleep(10 * 1000);
        mpp_log("%03d:\n", i);
        ctx->notify((void*)ctx);
    }

    event_ctx_release(ctx);

    return 0;
}
#endif
