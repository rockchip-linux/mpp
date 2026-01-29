/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "H265d"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_packet_impl.h"
#include "rk_hdr_meta_com.h"

#include "mpp_parser.h"
#include "mpp_ring.h"
#include "h265d_debug.h"
#include "h265d_ctx.h"
#include "h265d_syntax.h"

#define MAX_FRAME_SIZE          2048000
#define START_CODE              0x000001
#define INIT_RING_SIZE          (512 * 1024)

#define MPP_PARSER_PTS_NB       4
#define H265D_STREAM_NOT_END    (-10)
#define MAX_PENDING_FRAMES      4

typedef struct FrameSeg_t {
    RK_U32 data_start;
    RK_U32 data_size;
} FrameSeg;

typedef struct H265dSpl_t {
    MppRing ring;
    RK_U32 ring_size;
    RK_U32 write_pos;
    RK_U32 data_start;
    RK_U32 data_size;
    RK_U64 scan_state;
    RK_S32 nal_boundary_detected;
    RK_S32 overlap_bytes;
    RK_S32 overlap_offset;
    RK_S64 pts;
    RK_S64 dts;
    RK_S64 pic_offset;
    RK_S64 cur_offset;
    RK_S64 next_pic_offset;
    RK_S64 last_pts;
    RK_S64 last_dts;
    RK_S32 need_fetch_ts;

    RK_S32 cur_frame_start_index;
    RK_S64 frame_pos_offset[MPP_PARSER_PTS_NB];
    RK_U32 frame_pos_end[MPP_PARSER_PTS_NB];
    RK_S64 frame_pos_pts[MPP_PARSER_PTS_NB];
    RK_S64 frame_pos_dts[MPP_PARSER_PTS_NB];

    // Pending frames tracking (for FIFO sync)
    FrameSeg pending_frames[MAX_PENDING_FRAMES];
    RK_S32 pending_frame_count;

    // Track actual output data start position (for spliter_frame comparison)
    RK_U32 last_output_data_start;

    RK_S32 eos;
} H265dSpl;

RK_S32 h265d_spliter_init(H265dSpliter *ctx)
{
    H265dSpl *p = (H265dSpl *)ctx;
    RK_S32 ret;

    if (ctx == NULL) {
        mpp_loge_f("ctx is NULL\n");
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;

    p = mpp_calloc(H265dSpl, 1);
    if (NULL == p) {
        mpp_loge_f("malloc spliter failed\n");
        return MPP_ERR_MALLOC;
    }

    ret = mpp_ring_get(&p->ring, INIT_RING_SIZE, "h265d_spliter");
    if (ret < 0) {
        mpp_loge_f("mpp_ring_get failed\n");
        mpp_free(p);
        return ret;
    }

    p->ring_size = INIT_RING_SIZE;
    p->write_pos = 0;
    p->data_start = 0;
    p->data_size = 0;
    p->need_fetch_ts = 1;
    p->pending_frame_count = 0;

    *ctx = p;

    return MPP_OK;
}

RK_S32 h265d_spliter_deinit(H265dSpliter ctx)
{
    H265dSpl *p = (H265dSpl *)ctx;

    if (p) {
        if (p->ring)
            mpp_ring_put(p->ring);
        mpp_free(p);
    }

    return MPP_OK;
}

RK_S32 h265d_spliter_reset(H265dSpliter ctx)
{
    H265dSpl *p = (H265dSpl *)ctx;

    if (p) {
        p->write_pos = 0;
        p->data_start = 0;
        p->data_size = 0;
        p->scan_state = 0;
        p->nal_boundary_detected = 0;
        p->overlap_bytes = 0;
        p->overlap_offset = 0;
        p->pts = 0;
        p->dts = 0;
        p->pic_offset = 0;
        p->cur_offset = 0;
        p->next_pic_offset = 0;
        p->last_pts = 0;
        p->last_dts = 0;
        p->need_fetch_ts = 1;
        p->cur_frame_start_index = 0;
        memset(p->frame_pos_offset, 0, sizeof(p->frame_pos_offset));
        memset(p->frame_pos_end, 0, sizeof(p->frame_pos_end));
        memset(p->frame_pos_pts, 0, sizeof(p->frame_pos_pts));
        memset(p->frame_pos_dts, 0, sizeof(p->frame_pos_dts));
        p->pending_frame_count = 0;
        p->eos = 0;
    }

    return MPP_OK;
}

void h265d_spliter_set_eos(H265dSpliter ctx, RK_S32 eos)
{
    H265dSpl *p = (H265dSpl *)ctx;

    if (p)
        p->eos = eos;
}

RK_S64 h265d_spliter_get_pts(H265dSpliter ctx)
{
    H265dSpl *p = (H265dSpl *)ctx;

    return (p != NULL) ? p->pts : 0;
}

RK_S64 h265d_spliter_get_dts(H265dSpliter ctx)
{
    H265dSpl *p = (H265dSpl *)ctx;

    return (p != NULL) ? p->dts : 0;
}

RK_S32 h265d_find_frame_end(H265dSpliter ctx, const RK_U8 *buf, RK_S32 buf_size)
{
    H265dSpl *p = (H265dSpl *)ctx;
    RK_S32 i;

    for (i = 0; i < buf_size; i++) {
        RK_S32 nut;
        RK_S32 layer_id;

        p->scan_state = (p->scan_state << 8) | buf[i];

        if (((p->scan_state >> 3 * 8) & 0xFFFFFF) != START_CODE)
            continue;

        nut = (p->scan_state >> (2 * 8 + 1)) & 0x3F;
        layer_id  =  (((p->scan_state >> 2 * 8) & 0x01) << 5) + (((p->scan_state >> 1 * 8) & 0xF8) >> 3);

        if ((nut >= NAL_VPS && nut <= NAL_AUD) || nut == NAL_SEI_PREFIX ||
            (nut >= 41 && nut <= 44) || (nut >= 48 && nut <= 55)) {
            if (p->nal_boundary_detected && !layer_id) {
                p->nal_boundary_detected = 0;
                return i - 5;
            }
        } else if (nut <= NAL_RASL_R ||
                   (nut >= NAL_BLA_W_LP && nut <= NAL_CRA_NUT)) {
            RK_S32 first_slice_segment_in_pic_flag = buf[i] >> 7;

            if (first_slice_segment_in_pic_flag && !layer_id) {
                if (!p->nal_boundary_detected) {
                    p->nal_boundary_detected = 1;
                } else {
                    p->nal_boundary_detected = 0;
                    return i - 5;
                }
            }
        }
    }

    return H265D_STREAM_NOT_END;
}

static RK_S32 h265d_combine_frame(H265dSpl *p, RK_S32 next, const RK_U8 **buf, RK_S32 *buf_size)
{
    void *ring_ptr;
    RK_U32 output_size = 0;

    h265d_dbg_split("combine_frame next %d buf_size %d data_size %u write_pos %u data_start %u\n",
                    next, *buf_size, p->data_size, p->write_pos, p->data_start);

    if (p->overlap_bytes) {
        h265d_dbg_split("overlap %d bytes state %llx pos %d offset %d\n",
                        p->overlap_bytes, p->scan_state & 0xFFFFFFFF, next, p->overlap_offset);
        h265d_dbg_split("overlap data %X %X %X %X\n", (*buf)[0], (*buf)[1], (*buf)[2], (*buf)[3]);
    }

    if (!*buf_size && next == H265D_STREAM_NOT_END)
        next = 0;

    if (next == H265D_STREAM_NOT_END) {
        RK_U32 needed = p->data_size + *buf_size + MPP_INPUT_BUFFER_PADDING_SIZE;

        h265d_dbg_split("accumulate data_size %u buf_size %d needed %u ring_size %u\n",
                        p->data_size, *buf_size, needed, p->ring_size);

        if (needed > p->ring_size) {
            RK_U32 old_size = p->ring_size;
            RK_U32 new_size = MPP_MAX(needed * 3 / 2, p->ring_size * 2);
            void *old_ring_ptr = mpp_ring_get_ptr(p->ring);

            h265d_dbg_split("resize ring_size %u -> %u\n", old_size, new_size);
            h265d_dbg_split("resize before data_start %u data_size %u write_pos %u\n",
                            p->data_start, p->data_size, p->write_pos);

            // Check if data wraps around the old ring boundary BEFORE resize
            RK_U8 *saved_wrap_data = NULL;
            RK_U32 wrap_size = 0;

            if (p->data_start + p->data_size > old_size) {
                RK_U32 tail_size = old_size - p->data_start;
                wrap_size = p->data_size - tail_size;

                h265d_dbg_split("resize wrapped data_start %u data_size %u tail %u wrap %u\n",
                                p->data_start, p->data_size, tail_size, wrap_size);

                // Save wrapped data from OLD ring pointer BEFORE resize
                saved_wrap_data = mpp_calloc(RK_U8, wrap_size);
                if (saved_wrap_data) {
                    memcpy(saved_wrap_data, (RK_U8 *)old_ring_ptr, wrap_size);
                    h265d_dbg_split("resize saved wrap_data [%u:%u] from old_ring_ptr %p\n",
                                    0, wrap_size, old_ring_ptr);
                } else {
                    mpp_loge_f("hal: alloc saved_wrap_data %u failed\n", wrap_size);
                }
            }

            // Resize first to get new memory
            if (mpp_ring_resize(p->ring, new_size)) {
                mpp_loge_f("hal: resize ring to %u failed\n", new_size);
                if (saved_wrap_data)
                    mpp_free(saved_wrap_data);
                return MPP_ERR_NOMEM;
            }
            p->ring_size = new_size;

            // Get new pointer after resize
            ring_ptr = mpp_ring_get_ptr(p->ring);
            h265d_dbg_split("resize old_ring_ptr %p new_ring_ptr %p\n",
                            old_ring_ptr, ring_ptr);

            // Verify data integrity after resize and move wrapped data
            if (saved_wrap_data && wrap_size > 0) {
                // Verify: data at new ring_ptr should match saved data
                if (memcmp(ring_ptr, saved_wrap_data, wrap_size) == 0) {
                    h265d_dbg_split("resize verify memfd data preserved after resize\n");
                } else {
                    mpp_loge_f("hal: resize memfd data corrupted\n");
                }

                // Move wrapped data to make it contiguous
                memcpy(ring_ptr + old_size, ring_ptr, wrap_size);
                p->write_pos = old_size + wrap_size;

                // Verify: moved data matches saved data
                if (memcmp(ring_ptr + old_size, saved_wrap_data, wrap_size) == 0) {
                    h265d_dbg_split("resize verify move success\n");
                } else {
                    mpp_loge_f("hal: resize move data corrupted\n");
                }

                h265d_dbg_split("resize move [%u:%u] -> [%u:%u]\n",
                                0, wrap_size, old_size, old_size + wrap_size);
                h265d_dbg_split("resize after data_start %u write_pos %u\n",
                                p->data_start, p->write_pos);

                mpp_free(saved_wrap_data);
            } else {
                h265d_dbg_split("resize no_wrap data_start %u data_size %u\n",
                                p->data_start, p->data_size);
            }
        }

        ring_ptr = mpp_ring_get_ptr(p->ring);
        memcpy(ring_ptr + p->write_pos, *buf, *buf_size);
        p->write_pos += *buf_size;
        if (p->write_pos >= p->ring_size)
            p->write_pos -= p->ring_size;
        p->data_size += *buf_size;

        h265d_dbg_split("accumulate done data_size %u write_pos %u\n", p->data_size, p->write_pos);

        return -1;
    }

    // Frame boundary found - need to output frame
    h265d_dbg_split("frame_boundary next %d data_size %u\n", next, p->data_size);

    ring_ptr = mpp_ring_get_ptr(p->ring);

    if (p->data_size) {
        RK_S32 frame_size = p->data_size + next;  // next can be negative

        h265d_dbg_split("output frame_size %d data_size %u next %d\n",
                        frame_size, p->data_size, next);

        // Save frame data location before modifying ring state
        RK_U8 *frame_data = ring_ptr + p->data_start;
        RK_U32 output_data_start = p->data_start;  // Save for pending_frames

        // Append new data (first 'next' bytes) to ring for current frame
        if (next > 0) {
            memcpy(ring_ptr + p->write_pos, *buf, next);
            p->write_pos += next;
            if (p->write_pos >= p->ring_size) {
                h265d_dbg_split("write_pos_wrap before %u -> after %u\n",
                                p->write_pos, p->write_pos - p->ring_size);
                p->write_pos -= p->ring_size;
            }
            p->data_size += next;
            h265d_dbg_split("append bytes %d data_size %u write_pos %u\n",
                            next, p->data_size, p->write_pos);
        }

        // Save remaining new input data for next frame
        // NOTE: For zero-copy, remaining data stays in input buffer (not copied to ring)
        // This prevents overwriting the zero-copy output frame
        if (next > 0 && *buf_size > next) {
            // Remaining data stays in input buffer
            // Don't copy to ring, just clear ring state
            p->data_size = 0;
            p->data_start = 0;
            p->write_pos = 0;
            h265d_dbg_split("keep_remain in_input_buf %u\n", *buf_size - next);
        } else {
            p->data_size = 0;
            p->data_start = 0;
            p->write_pos = 0;
            h265d_dbg_split("ring_clear data_size 0 data_start 0 write_pos 0\n");
        }

        // Zero-copy output: directly return ring pointer
        // Save frame info for sync release (FIFO)
        if (p->pending_frame_count < MAX_PENDING_FRAMES) {
            p->pending_frames[p->pending_frame_count].data_start = output_data_start;
            p->pending_frames[p->pending_frame_count].data_size = frame_size;
            p->pending_frame_count++;
        }

        *buf = frame_data;
        *buf_size = p->overlap_offset = frame_size;

        // Save actual output data start for spliter_frame comparison
        p->last_output_data_start = output_data_start;

        h265d_dbg_split("zero_copy_output frame_size %d pending_count %d output_data_start %u\n",
                        frame_size, p->pending_frame_count, output_data_start);
    } else {
        // No accumulated data - output new input directly
        output_size = next;
        *buf_size = p->overlap_offset = next;
        h265d_dbg_split("no_accum output_size %d\n", next);
    }

    if (next < -8) {
        p->overlap_bytes += (-8 - next);
        next = -8;
        h265d_dbg_split("overlap_adjust overlap_bytes %d next %d\n", p->overlap_bytes, next);
    }

    // Calculate overlap position for scan_state update
    // Scan from the output frame data
    RK_U32 scan_pos = 0;
    if (p->data_size) {
        scan_pos = p->data_size;
    } else {
        scan_pos = output_size;
    }

    while (next < 0) {
        RK_S32 pos = scan_pos + next;
        const RK_U8 *data_ptr;

        if (p->data_size || output_size) {
            // Read from output frame (from *buf which was set above)
            data_ptr = (const RK_U8 *)*buf;
        } else {
            ring_ptr = mpp_ring_get_ptr(p->ring);
            data_ptr = (const RK_U8 *)ring_ptr + p->data_start;
        }
        p->scan_state = (p->scan_state << 8) | data_ptr[pos];
        p->overlap_bytes++;
        next++;
    }

    if (p->overlap_bytes) {
        h265d_dbg_split("overlap_final bytes %d state %llX\n",
                        p->overlap_bytes, p->scan_state & 0xFFFFFFFF);
    }

    return 0;
}

void h265d_spliter_sync(H265dSpliter ctx, RK_S32 frame_id)
{
    H265dSpl *p = (H265dSpl *)ctx;

    if (!p || p->pending_frame_count == 0)
        return;

    (void)frame_id;  // frame_id is not used in FIFO mode

    h265d_dbg_split("sync pending_count %d\n", p->pending_frame_count);

    // FIFO: Release the oldest frame (index 0)
    RK_U32 old_data_start = p->pending_frames[0].data_start;
    RK_U32 old_data_size = p->pending_frames[0].data_size;

    // Skip the released frame's data from data_start
    // Note: data_size is already updated during output, only update data_start here
    RK_U32 prev_data_start = p->data_start;
    p->data_start += old_data_size;
    if (p->data_start >= p->ring_size)
        p->data_start -= p->ring_size;

    // Sync write_pos with data_start to maintain ring buffer consistency
    // When data_size is 0 (entire frame was output), ring was cleared.
    // In this case, sync write_pos to match data_start for next accumulation.
    if (p->data_size == 0) {
        p->write_pos = p->data_start;
    } else if (p->data_start > prev_data_start && p->write_pos < p->data_start) {
        // data_start moved forward without wrap, sync write_pos if needed
        p->write_pos = p->data_start;
    }

    h265d_dbg_split("sync_release old_start %u old_size %u new_start %u data_size %u write_pos %u\n",
                    old_data_start, old_data_size, p->data_start, p->data_size, p->write_pos);

    // Remove processed frame from pending_frames (shift left)
    for (RK_S32 i = 0; i < p->pending_frame_count - 1; i++) {
        p->pending_frames[i] = p->pending_frames[i + 1];
    }
    p->pending_frame_count--;
}

static void h265d_get_timestamp(H265dSpl *p, RK_S32 off)
{
    RK_S32 i;

    p->pts = -1;
    p->dts = -1;

    for (i = 0; i < MPP_PARSER_PTS_NB; i++) {
        h265d_dbg_pts("cur_offset %lld frame_pos_offset[%d] %lld pic_offset %lld next_pic_offset %lld",
                      p->cur_offset, i, p->frame_pos_offset[i], p->pic_offset, p->next_pic_offset);
        if (p->cur_offset + off >= p->frame_pos_offset[i]
            && (p->pic_offset < p->frame_pos_offset[i] ||
                (!p->pic_offset && !p->next_pic_offset))
            &&  p->frame_pos_end[i]) {
            p->dts = p->frame_pos_dts[i];
            p->pts = p->frame_pos_pts[i];
            if (p->cur_offset + off < p->frame_pos_end[i])
                break;
        }
    }
}

RK_S32 h265d_spliter_frame(H265dSpliter ctx, const RK_U8 **buf_out, RK_S32 *buf_size_out,
                           const RK_U8 *buf, RK_S32 buf_size, RK_S64 pts, RK_S64 dts)
{
    H265dSpl *p = (H265dSpl*)ctx;
    RK_S32 next;
    RK_S32 i;

    if (p->cur_offset + buf_size !=
        p->frame_pos_end[p->cur_frame_start_index]) {
        i = p->cur_frame_start_index + 1;

        if (i >= MPP_PARSER_PTS_NB)
            i = 0;

        p->cur_frame_start_index = i;
        p->frame_pos_offset[i] = p->cur_offset;
        p->frame_pos_end[i] = p->cur_offset + buf_size;
        p->frame_pos_pts[i] = pts;
        p->frame_pos_dts[i] = dts;

        h265d_dbg_pts("cur_frame_start_index %d frame_pos_offset %lld frame_pos_end %lld pts %lld",
                      p->cur_frame_start_index, p->frame_pos_offset[i], p->frame_pos_end[i], pts);
    }

    if (p->need_fetch_ts) {
        p->need_fetch_ts = 0;
        p->last_pts = p->pts;
        p->last_dts = p->dts;
        h265d_get_timestamp(p, 0);
    }

    if (p->eos && !buf_size) {
        void *ring_ptr = mpp_ring_get_ptr(p->ring);

        *buf_out = ring_ptr + p->data_start;
        *buf_size_out = p->data_size;
        return 0;
    }

    next = h265d_find_frame_end(p, buf, buf_size);
    if (p->eos && buf_size && next == H265D_STREAM_NOT_END) {
        next = buf_size;
    }

    // old_data_start is no longer used - we use last_output_data_start from combine_frame

    if (h265d_combine_frame(p, next, &buf, &buf_size) < 0) {
        *buf_out = NULL;
        *buf_size_out = 0;
        p->cur_offset += buf_size;
        return buf_size;
    }

    *buf_out = buf;
    *buf_size_out = buf_size;

    if (next < 0)
        next = 0;

    if (*buf_size_out) {
        void *ring_ptr;
        p->pic_offset = p->next_pic_offset;
        p->next_pic_offset = p->cur_offset + next;
        p->need_fetch_ts = 1;

        // Check if output is from temporary buffer or ring
        ring_ptr = mpp_ring_get_ptr(p->ring);
        if ((const RK_U8 *)*buf_out < (const RK_U8 *)ring_ptr ||
            (const RK_U8 *)*buf_out >= (const RK_U8 *)ring_ptr + p->ring_size * 2) {
            // Output is from temporary buffer - ring state was already updated
            h265d_dbg_split("output_src temp_buf data_size %u data_start %u\n",
                            p->data_size, p->data_start);
        } else if ((const RK_U8 *)*buf_out == (const RK_U8 *)ring_ptr + p->last_output_data_start) {
            // Output is from old ring position (normal frame output)
            // Ring state was already updated in h265d_combine_frame
            h265d_dbg_split("output_src old_ring data_size %u data_start %u\n",
                            p->data_size, p->data_start);
        } else {
            // Output is from current ring position (EOS case) - need to update ring state
            RK_U32 old_data_size = p->data_size;
            RK_U32 old_data_start = p->data_start;
            p->data_size -= *buf_size_out;
            p->data_start += *buf_size_out;
            if (p->data_start >= p->ring_size)
                p->data_start -= p->ring_size;
            h265d_dbg_split("output_src curr_ring data_size %u->%u data_start %u->%u\n",
                            old_data_size, p->data_size, old_data_start, p->data_start);
        }
    }

    p->cur_offset += next;

    return next;
}
