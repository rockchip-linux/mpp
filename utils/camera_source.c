/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "camera_source"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/select.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "camera_source.h"

typedef struct CamFrame_t {
    void        *start;
    size_t      length;
    RK_S32      export_fd;
    RK_S32      sequence;
    MppBuffer   buffer;
} CamFrame;

struct CamSource {
    RK_S32              fd;     // Device handle
    RK_U32              bufcnt; // # of buffers
    enum v4l2_buf_type  type;
    MppFrameFormat      fmt;
    CamFrame            fbuf[10];// frame buffers
};

static RK_U32 V4L2_yuv_cfg[MPP_FMT_YUV_BUTT] = {
    V4L2_PIX_FMT_NV12,
    0,
    V4L2_PIX_FMT_NV16,
    0,
    V4L2_PIX_FMT_YVU420,
    V4L2_PIX_FMT_NV21,
    V4L2_PIX_FMT_YUV422P,
    V4L2_PIX_FMT_NV61,
    V4L2_PIX_FMT_YUYV,
    V4L2_PIX_FMT_YVYU,
    V4L2_PIX_FMT_UYVY,
    V4L2_PIX_FMT_VYUY,
    V4L2_PIX_FMT_GREY,
    0,
    0,
    0,
};

static RK_U32 V4L2_RGB_cfg[MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB] = {
    V4L2_PIX_FMT_RGB565,
    0,
    V4L2_PIX_FMT_RGB555,
    0,
    V4L2_PIX_FMT_RGB444,
    0,
    V4L2_PIX_FMT_RGB24,
    V4L2_PIX_FMT_BGR24,
    0,
    0,
    V4L2_PIX_FMT_RGB32,
    V4L2_PIX_FMT_BGR32,
    0,
    0,
};

#define FMT_NUM_PLANES 1

// Wrap ioctl() to spin on EINTR
static RK_S32 camera_source_ioctl(RK_S32 fd, RK_S32 req, void* arg)
{
    struct timespec poll_time;
    RK_S32 ret;

    while ((ret = ioctl(fd, req, arg))) {
        if (ret == -1 && (EINTR != errno && EAGAIN != errno)) {
            // mpp_err("ret = %d, errno %d", ret, errno);
            break;
        }
        // 10 milliseconds
        poll_time.tv_sec = 0;
        poll_time.tv_nsec = 10000000;
        nanosleep(&poll_time, NULL);
    }

    return ret;
}

// Create a new context to capture frames from <fname>.
// Returns NULL on error.
CamSource *camera_source_init(const char *device, RK_U32 bufcnt, RK_U32 width, RK_U32 height, MppFrameFormat format)
{
    struct v4l2_capability     cap;
    struct v4l2_format         vfmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer         buf;
    enum   v4l2_buf_type       type;
    RK_U32 i;
    RK_U32 buf_len = 0;
    CamSource *ctx;

    ctx = mpp_calloc(CamSource, 1);
    if (!ctx)
        return NULL;

    ctx->bufcnt = bufcnt;
    ctx->fd = open(device, O_RDWR | O_CLOEXEC, 0);
    if (ctx->fd < 0) {
        mpp_err_f("Cannot open device\n");
        goto FAIL;
    }

    {
        struct v4l2_input input;

        input.index = 0;
        while (!camera_source_ioctl(ctx->fd, VIDIOC_ENUMINPUT, &input)) {
            mpp_log("input devices:%s\n", input.name);
            ++input.index;
        }
    }

    // Determine if fd is a V4L2 Device
    if (0 != camera_source_ioctl(ctx->fd, VIDIOC_QUERYCAP, &cap)) {
        mpp_err_f("Not v4l2 compatible\n");
        goto FAIL;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) && !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
        mpp_err_f("Capture not supported\n");
        goto FAIL;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        mpp_err_f("Streaming IO Not Supported\n");
        goto FAIL;
    }

    // Preserve original settings as set by v4l2-ctl for example
    vfmt = (struct v4l2_format) {0};
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
        vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    vfmt.fmt.pix.width = width;
    vfmt.fmt.pix.height = height;

    {
        struct v4l2_fmtdesc fmtdesc;

        fmtdesc.index = 0;
        fmtdesc.type = vfmt.type;
        while (!camera_source_ioctl(ctx->fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
            mpp_log("fmt name: [%s]\n", fmtdesc.description);
            mpp_log("fmt pixelformat: '%c%c%c%c', description = '%s'\n", fmtdesc.pixelformat & 0xFF,
                    (fmtdesc.pixelformat >> 8) & 0xFF, (fmtdesc.pixelformat >> 16) & 0xFF,
                    (fmtdesc.pixelformat >> 24) & 0xFF, fmtdesc.description);
            fmtdesc.index++;
        }
    }

    if (MPP_FRAME_FMT_IS_YUV(format)) {
        vfmt.fmt.pix.pixelformat = V4L2_yuv_cfg[format - MPP_FRAME_FMT_YUV];
    } else if (MPP_FRAME_FMT_IS_RGB(format)) {
        vfmt.fmt.pix.pixelformat = V4L2_RGB_cfg[format - MPP_FRAME_FMT_RGB];
    }

    if (!vfmt.fmt.pix.pixelformat)
        vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;

    type = vfmt.type;
    ctx->type = vfmt.type;

    if (-1 == camera_source_ioctl(ctx->fd, VIDIOC_S_FMT, &vfmt)) {
        mpp_err_f("VIDIOC_S_FMT\n");
        goto FAIL;
    }

    if (-1 == camera_source_ioctl(ctx->fd, VIDIOC_G_FMT, &vfmt)) {
        mpp_err_f("VIDIOC_G_FMT\n");
        goto FAIL;
    }

    mpp_log("get width %d height %d", vfmt.fmt.pix.width, vfmt.fmt.pix.height);

    // Request memory-mapped buffers
    req = (struct v4l2_requestbuffers) {0};
    req.count  = ctx->bufcnt;
    req.type   = type;
    req.memory = V4L2_MEMORY_MMAP;
    if (-1 == camera_source_ioctl(ctx->fd, VIDIOC_REQBUFS, &req)) {
        mpp_err_f("Device does not support mmap\n");
        goto FAIL;
    }

    if (req.count != ctx->bufcnt) {
        mpp_err_f("Device buffer count mismatch\n");
        goto FAIL;
    }

    // mmap() the buffers into userspace memory
    for (i = 0 ; i < ctx->bufcnt; i++) {
        buf = (struct v4l2_buffer) {0};
        buf.type    = type;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = i;
        struct v4l2_plane planes[FMT_NUM_PLANES];
        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == type) {
            buf.m.planes = planes;
            buf.length = FMT_NUM_PLANES;
        }

        if (-1 == camera_source_ioctl(ctx->fd, VIDIOC_QUERYBUF, &buf)) {
            mpp_err_f("ERROR: VIDIOC_QUERYBUF\n");
            goto FAIL;
        }

        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == buf.type) {
            // tmp_buffers[n_buffers].length = buf.m.planes[0].length;
            buf_len = buf.m.planes[0].length;
            ctx->fbuf[i].start =
                mmap(NULL /* start anywhere */,
                     buf.m.planes[0].length,
                     PROT_READ | PROT_WRITE /* required */,
                     MAP_SHARED /* recommended */,
                     ctx->fd, buf.m.planes[0].m.mem_offset);
        } else {
            buf_len = buf.length;
            ctx->fbuf[i].start =
                mmap(NULL /* start anywhere */,
                     buf.length,
                     PROT_READ | PROT_WRITE /* required */,
                     MAP_SHARED /* recommended */,
                     ctx->fd, buf.m.offset);
        }
        if (MAP_FAILED == ctx->fbuf[i].start) {
            mpp_err_f("ERROR: Failed to map device frame buffers\n");
            goto FAIL;
        }

        ctx->fbuf[i].length = buf_len; // record buffer length for unmap

        struct v4l2_exportbuffer expbuf = (struct v4l2_exportbuffer) {0} ;
        // xcam_mem_clear (expbuf);
        expbuf.type = type;
        expbuf.index = i;
        expbuf.flags = O_CLOEXEC;
        if (camera_source_ioctl(ctx->fd, VIDIOC_EXPBUF, &expbuf) < 0) {
            mpp_err_f("get dma buf failed\n");
            goto FAIL;
        } else {
            mpp_log("get dma buf(%d)-fd: %d\n", i, expbuf.fd);
            MppBufferInfo info;
            memset(&info, 0, sizeof(MppBufferInfo));
            info.type = MPP_BUFFER_TYPE_EXT_DMA;
            info.fd =  expbuf.fd;
            info.size = buf_len & 0x07ffffff;
            info.index = (buf_len & 0xf8000000) >> 27;
            mpp_buffer_import(&ctx->fbuf[i].buffer, &info);
        }
        ctx->fbuf[i].export_fd = expbuf.fd;
    }

    for (i = 0; i < ctx->bufcnt; i++ ) {
        struct v4l2_plane planes[FMT_NUM_PLANES];

        buf = (struct v4l2_buffer) {0};
        buf.type    = type;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = i;

        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == type) {
            buf.m.planes = planes;
            buf.length = FMT_NUM_PLANES;
        }

        if (-1 == camera_source_ioctl(ctx->fd, VIDIOC_QBUF, &buf)) {
            mpp_err_f("ERROR: VIDIOC_QBUF %d\n", i);
            goto FAIL;
        }
    }

    // Start capturing
    if (-1 == camera_source_ioctl(ctx->fd, VIDIOC_STREAMON, &type)) {
        mpp_err_f("ERROR: VIDIOC_STREAMON\n");
        goto FAIL;
    }

    //skip some frames at start
    for (i = 0; i < ctx->bufcnt; i++ ) {
        RK_S32 idx = camera_source_get_frame(ctx);
        if (idx >= 0)
            camera_source_put_frame(ctx, idx);
    }

    return ctx;

FAIL:
    camera_source_deinit(ctx);
    return NULL;
}

// Free a context to capture frames from <fname>.
// Returns NULL on error.
MPP_RET camera_source_deinit(CamSource *ctx)
{
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;
    RK_U32 i;

    if (NULL == ctx)
        return MPP_OK;

    if (ctx->fd < 0)
        return MPP_OK;

    // Stop capturing
    type = ctx->type;

    camera_source_ioctl(ctx->fd, VIDIOC_STREAMOFF, &type);

    // un-mmap() buffers
    for (i = 0 ; i < ctx->bufcnt; i++) {
        buf = (struct v4l2_buffer) {0};
        buf.type    = type;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = i;
        camera_source_ioctl(ctx->fd, VIDIOC_QUERYBUF, &buf);
        if (ctx->fbuf[buf.index].buffer) {
            mpp_buffer_put(ctx->fbuf[buf.index].buffer);
        }
        munmap(ctx->fbuf[buf.index].start, ctx->fbuf[buf.index].length);
        close(ctx->fbuf[i].export_fd);
    }

    // Close v4l2 device
    close(ctx->fd);
    MPP_FREE(ctx);
    return MPP_OK;
}

// Returns a pointer to a captured frame and its meta-data. NOT thread-safe.
RK_S32 camera_source_get_frame(CamSource *ctx)
{
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;

    type = ctx->type;
    buf = (struct v4l2_buffer) {0};
    buf.type   = type;
    buf.memory = V4L2_MEMORY_MMAP;

    struct v4l2_plane planes[FMT_NUM_PLANES];
    if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == type) {
        buf.m.planes = planes;
        buf.length = FMT_NUM_PLANES;
    }

    if (-1 == camera_source_ioctl(ctx->fd, VIDIOC_DQBUF, &buf)) {
        mpp_err_f("VIDIOC_DQBUF\n");
        return -1;
    }

    if (buf.index > ctx->bufcnt) {
        mpp_err_f("buffer index out of bounds\n");
        return -1;
    }

    if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == type)
        buf.bytesused = buf.m.planes[0].bytesused;

    return buf.index;
}

// It's OK to capture into this framebuffer now
MPP_RET camera_source_put_frame(CamSource *ctx, RK_S32 idx)
{
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;

    if (idx < 0)
        return MPP_OK;

    type = ctx->type;
    buf = (struct v4l2_buffer) {0};
    buf.type   = type;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index  = idx;

    struct v4l2_plane planes[FMT_NUM_PLANES];
    if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == type) {
        buf.m.planes = planes;
        buf.length = FMT_NUM_PLANES;
    }

    // Tell kernel it's ok to overwrite this frame
    if (-1 == camera_source_ioctl(ctx->fd, VIDIOC_QBUF, &buf)) {
        mpp_err_f("VIDIOC_QBUF\n");
        return MPP_OK;
    }

    return MPP_OK;
}

MppBuffer camera_frame_to_buf(CamSource *ctx, RK_S32 idx)
{
    MppBuffer buf = NULL;

    if (idx < 0)
        return buf;

    buf = ctx->fbuf[idx].buffer;
    if (buf)
        mpp_buffer_sync_end(buf);

    return buf;
}
