#include "mpp_log.h"
#include "mpp_time.h"
#include "mpp_thread.h"

#include "mpp_task.h"
#include "mpp_task_impl.h"

#define MAX_TASK_LOOP   10000

static MppTaskQueue input  = NULL;
static MppTaskQueue output = NULL;

void *task_input(void *arg)
{
    RK_S32 i;
    MppTask task = NULL;
    MPP_RET ret = MPP_OK;
    MppPort port = mpp_task_queue_get_port(input, MPP_PORT_INPUT);
    MppFrame frm;

    mpp_frame_init(&frm);

    for (i = 0; i < MAX_TASK_LOOP; i++) {
        ret = mpp_port_poll(port, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port, task);
        mpp_assert(!ret);
    }

    mpp_frame_deinit(&frm);

    (void)arg;
    return NULL;
}

void *task_output(void *arg)
{
    RK_S32 i;
    MppTask task = NULL;
    MPP_RET ret = MPP_OK;
    MppPort port = mpp_task_queue_get_port(output, MPP_PORT_OUTPUT);
    MppPacket pkt;

    mpp_packet_init(&pkt, NULL, 0);

    for (i = 0; i < MAX_TASK_LOOP; i++) {
        ret = mpp_port_poll(port, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port, task);
        mpp_assert(!ret);
    }

    mpp_packet_deinit(&pkt);

    (void)arg;
    return NULL;
}

void *task_in_and_out(void *arg)
{
    RK_S32 i;
    MppTask task = NULL;
    MPP_RET ret = MPP_OK;
    MppPort port_input  = mpp_task_queue_get_port(input, MPP_PORT_INPUT);
    MppPort port_output = mpp_task_queue_get_port(output, MPP_PORT_OUTPUT);

    for (i = 0; i < MAX_TASK_LOOP; i++) {
        ret = mpp_port_poll(port_input, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port_input, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port_input, task);
        mpp_assert(!ret);


        ret = mpp_port_poll(port_output, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port_output, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port_output, task);
        mpp_assert(!ret);
    }

    (void)arg;
    return NULL;
}

void *task_worker(void *arg)
{
    RK_S32 i;
    MppTask task = NULL;
    MPP_RET ret = MPP_OK;
    MppPort port_src = mpp_task_queue_get_port(input, MPP_PORT_OUTPUT);
    MppPort port_dst = mpp_task_queue_get_port(output, MPP_PORT_INPUT);

    for (i = 0; i < MAX_TASK_LOOP; i++) {
        ret = mpp_port_poll(port_src, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port_src, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port_src, task);
        mpp_assert(!ret);

        ret = mpp_port_poll(port_dst, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port_dst, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port_dst, task);
        mpp_assert(!ret);
    }

    (void)arg;
    return NULL;
}

void serial_task(void)
{
    RK_S32 i;
    MppTask task = NULL;
    MPP_RET ret = MPP_OK;
    MppPort port_ii = mpp_task_queue_get_port(input, MPP_PORT_INPUT);
    MppPort port_io = mpp_task_queue_get_port(input, MPP_PORT_OUTPUT);
    MppPort port_oi = mpp_task_queue_get_port(output, MPP_PORT_INPUT);
    MppPort port_oo = mpp_task_queue_get_port(output, MPP_PORT_OUTPUT);

    for (i = 0; i < MAX_TASK_LOOP; i++) {
        ret = mpp_port_poll(port_ii, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port_ii, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port_ii, task);
        mpp_assert(!ret);


        ret = mpp_port_poll(port_io, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port_io, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port_io, task);
        mpp_assert(!ret);


        ret = mpp_port_poll(port_oi, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port_oi, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port_oi, task);
        mpp_assert(!ret);


        ret = mpp_port_poll(port_oo, MPP_POLL_BLOCK);
        mpp_assert(!ret);

        ret = mpp_port_dequeue(port_oo, &task);
        mpp_assert(!ret);
        mpp_assert(task);

        ret = mpp_port_enqueue(port_oo, task);
        mpp_assert(!ret);
    }
}

int main()
{
    RK_S64 time_start, time_end;

    pthread_t thread_input;
    pthread_t thread_output;
    pthread_t thread_in_and_out;
    pthread_t thread_worker;
    pthread_attr_t attr;
    void *dummy;

    mpp_log("mpp task test start\n");

    mpp_task_queue_init(&input);
    mpp_task_queue_init(&output);
    mpp_task_queue_setup(input, 4);
    mpp_task_queue_setup(output, 4);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    mpp_debug = MPP_DBG_TIMING;

    time_start = mpp_time();
    pthread_create(&thread_input,  &attr, task_input,  NULL);
    pthread_create(&thread_output, &attr, task_output, NULL);
    pthread_create(&thread_worker, &attr, task_worker, NULL);

    pthread_join(thread_input, &dummy);
    pthread_join(thread_worker, &dummy);
    pthread_join(thread_output, &dummy);
    time_end = mpp_time();
    mpp_time_diff(time_start, time_end, 0, "3 thread test");

    time_start = mpp_time();
    pthread_create(&thread_in_and_out,  &attr, task_in_and_out,  NULL);
    pthread_create(&thread_worker, &attr, task_worker, NULL);

    pthread_join(thread_in_and_out, &dummy);
    pthread_join(thread_worker, &dummy);
    time_end = mpp_time();
    mpp_time_diff(time_start, time_end, 0, "2 thread test");
    pthread_attr_destroy(&attr);

    time_start = mpp_time();
    serial_task();
    time_end = mpp_time();
    mpp_time_diff(time_start, time_end, 0, "1 thread test");

    mpp_debug = 0;

    mpp_task_queue_deinit(input);
    mpp_task_queue_deinit(output);

    mpp_log("mpp task test done\n");

    return 0;
}

