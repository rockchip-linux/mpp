/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Rockchip Products .                             --
--                                                                            --
--                   (C) COPYRIGHT 2014 ROCKCHIP PRODUCTS                     --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
-- Description : H.264 SEI Messages.
--
--------------------------------------------------------------------------------
--
--
------------------------------------------------------------------------------*/

#include "H264Sei.h"
#include "H264PutBits.h"
#include "H264NalUnit.h"
#include "enccommon.h"

#define SEI_BUFFERING_PERIOD        0
#define SEI_PIC_TIMING              1
#define SEI_FILLER_PAYLOAD          3
#define SEI_USER_DATA_UNREGISTERED  5

/*------------------------------------------------------------------------------
  H264InitSei()
------------------------------------------------------------------------------*/
void H264InitSei(sei_s * sei, true_e byteStream, u32 hrd, u32 timeScale,
                 u32 nuit)
{
    ASSERT(sei != NULL);

    sei->byteStream = byteStream;
    sei->hrd = hrd;
    sei->seqId = 0x0;
    sei->psp = (u32) ENCHW_YES;
    sei->cts = (u32) ENCHW_YES;
    /* sei->icrd = 0; */
    sei->icrdLen = 24;
    /* sei->icrdo = 0; */
    sei->icrdoLen = 24;
    /* sei->crd = 0; */
    sei->crdLen = 24;
    /* sei->dod = 0; */
    sei->dodLen = 24;
    sei->ps = 0;
    sei->cntType = 1;
    sei->cdf = 0;
    sei->nframes = 0;
    sei->toffs = 0;

    {
        u32 n = 1;

        while (nuit > (1U << n))
            n++;
        sei->toffsLen = n;
    }

    sei->ts.timeScale = timeScale;
    sei->ts.nuit = nuit;
    sei->ts.time = 0;
    sei->ts.sec = 0;
    sei->ts.min = 0;
    sei->ts.hr = 0;
    sei->ts.fts = (u32) ENCHW_YES;
    sei->ts.secf = (u32) ENCHW_NO;
    sei->ts.minf = (u32) ENCHW_NO;
    sei->ts.hrf = (u32) ENCHW_NO;

    sei->userDataEnabled = (u32) ENCHW_NO;
    sei->userDataSize = 0;
    sei->pUserData = NULL;
}

/*------------------------------------------------------------------------------
  H264UpdateSeiTS()  Calculate new time stamp.
------------------------------------------------------------------------------*/
void H264UpdateSeiTS(sei_s * sei, u32 timeInc)
{
    timeStamp_s *ts = &sei->ts;

    ASSERT(sei != NULL);
    timeInc += ts->time;

    while (timeInc >= ts->timeScale) {
        timeInc -= ts->timeScale;
        ts->sec++;
        if (ts->sec == 60) {
            ts->sec = 0;
            ts->min++;
            if (ts->min == 60) {
                ts->min = 0;
                ts->hr++;
                if (ts->hr == 32) {
                    ts->hr = 0;
                }
            }
        }
    }

    ts->time = timeInc;

    sei->nframes = ts->time / ts->nuit;
    sei->toffs = ts->time - sei->nframes * ts->nuit;

    ts->hrf = (ts->hr != 0);
    ts->minf = ts->hrf || (ts->min != 0);
    ts->secf = ts->minf || (ts->sec != 0);

#ifdef TRACE_PIC_TIMING
    DEBUG_PRINT(("Picture Timing: %02i:%02i:%02i  %6i ticks\n", ts->hr, ts->min,
                 ts->sec, (sei->nframes * ts->nuit + sei->toffs)));
#endif
}

/*------------------------------------------------------------------------------
  H264FillerSei()  Filler payload SEI message. Requested filler payload size
  could be huge. Use of temporary stream buffer is not needed, because size is
  know.
------------------------------------------------------------------------------*/
void H264FillerSei(stream_s * sp, sei_s * sei, i32 cnt)
{
    i32 i = cnt;

    ASSERT(sp != NULL && sei != NULL);

    H264NalUnitHdr(sp, 0, SEI, sei->byteStream);

    H264NalBits(sp, SEI_FILLER_PAYLOAD, 8);
    COMMENT("last_payload_type_byte");

    while (cnt >= 255) {
        H264NalBits(sp, 0xFF, 0x8);
        COMMENT("ff_byte");
        cnt -= 255;
    }
    H264NalBits(sp, cnt, 8);
    COMMENT("last_payload_size_byte");

    for (; i > 0; i--) {
        H264NalBits(sp, 0xFF, 8);
        COMMENT("filler ff_byte");
    }
    H264RbspTrailingBits(sp);
}

/*------------------------------------------------------------------------------
  H264BufferingSei()  Buffering period SEI message.
------------------------------------------------------------------------------*/
void H264BufferingSei(stream_s * sp, sei_s * sei)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    if (sei->hrd == ENCHW_NO) {
        return;
    }

    H264NalBits(sp, SEI_BUFFERING_PERIOD, 8);
    COMMENT("last_payload_type_byte");

    pPayloadSizePos = sp->stream;

    H264NalBits(sp, 0xFF, 8);   /* this will be updated after we know exact payload size */
    COMMENT("last_payload_size_byte");

    startByteCnt = sp->byteCnt;
    sp->emulCnt = 0;    /* count emul_3_byte for this payload */

    H264ExpGolombUnsigned(sp, sei->seqId);
    COMMENT("seq_parameter_set_id");

    H264NalBits(sp, sei->icrd, sei->icrdLen);
    COMMENT("initial_cpb_removal_delay");

    H264NalBits(sp, sei->icrdo, sei->icrdoLen);
    COMMENT("initial_cpb_removal_delay_offset");

    if (sp->bufferedBits) {
        H264RbspTrailingBits(sp);
    }

    {
        u32 payload_size;

        payload_size = sp->byteCnt - startByteCnt - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }

    /* reset cpb_removal_delay */
    sei->crd = 0;
}

/*------------------------------------------------------------------------------
  TimeStamp()
------------------------------------------------------------------------------*/
static void TimeStamp(stream_s * sp, timeStamp_s * ts)
{
    if (ts->fts) {
        H264NalBits(sp, ts->sec, 6);
        COMMENT("seconds_value");
        H264NalBits(sp, ts->min, 6);
        COMMENT("minutes_value");
        H264NalBits(sp, ts->hr, 5);
        COMMENT("hours_value");
    } else {
        H264NalBits(sp, ts->secf, 1);
        COMMENT("seconds_flag");
        if (ts->secf) {
            H264NalBits(sp, ts->sec, 6);
            COMMENT("seconds_value");
            H264NalBits(sp, ts->minf, 1);
            COMMENT("minutes_flag");
            if (ts->minf) {
                H264NalBits(sp, ts->min, 6);
                COMMENT("minutes_value");
                H264NalBits(sp, ts->hrf, 1);
                COMMENT("hours_flag");
                if (ts->hrf) {
                    H264NalBits(sp, ts->hr, 5);
                    COMMENT("hours_value");
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------
  H264PicTimingSei()  Picture timing SEI message.
------------------------------------------------------------------------------*/
void H264PicTimingSei(stream_s * sp, sei_s * sei)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    H264NalBits(sp, SEI_PIC_TIMING, 8);
    COMMENT("last_payload_type_byte");

    pPayloadSizePos = sp->stream;

    H264NalBits(sp, 0xFF, 8);   /* this will be updated after we know exact payload size */
    COMMENT("last_payload_size_byte");

    startByteCnt = sp->byteCnt;
    sp->emulCnt = 0;    /* count emul_3_byte for this payload */

    if (sei->hrd) {
        H264NalBits(sp, sei->crd, sei->crdLen);
        COMMENT("cpb_removal_delay");
        H264NalBits(sp, sei->dod, sei->dodLen);
        COMMENT("dpb_output_delay");
    }

    if (sei->psp) {
        H264NalBits(sp, sei->ps, 4);
        COMMENT("pic_struct");
        H264NalBits(sp, sei->cts, 1);
        COMMENT("clock_timestamp_flag");
        if (sei->cts) {
            H264NalBits(sp, 0, 2);
            COMMENT("ct_type");
            H264NalBits(sp, 0, 1);
            COMMENT("nuit_field_based_flag");
            H264NalBits(sp, sei->cntType, 5);
            COMMENT("counting_type");
            H264NalBits(sp, sei->ts.fts, 1);
            COMMENT("full_timestamp_flag");
            H264NalBits(sp, 0, 1);
            COMMENT("discontinuity_flag");
            H264NalBits(sp, sei->cdf, 1);
            COMMENT("cnt_dropped_flag");
            H264NalBits(sp, sei->nframes, 8);
            COMMENT("n_frames");
            TimeStamp(sp, &sei->ts);
            if (sei->toffsLen > 0) {
                H264NalBits(sp, sei->toffs, sei->toffsLen);
                COMMENT("time_offset");
            }
        }
    }

    if (sp->bufferedBits) {
        H264RbspTrailingBits(sp);
    }

    {
        u32 payload_size;

        payload_size = sp->byteCnt - startByteCnt - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }
}

/*------------------------------------------------------------------------------
  H264UserDataUnregSei()  User data unregistered SEI message.
------------------------------------------------------------------------------*/
void H264UserDataUnregSei(stream_s * sp, sei_s * sei)
{
    u32 i, cnt;
    const u8 * pUserData;
    ASSERT(sei != NULL);
    ASSERT(sei->pUserData != NULL);
    ASSERT(sei->userDataSize >= 16);

    pUserData = sei->pUserData;
    cnt = sei->userDataSize;
    if (sei->userDataEnabled == ENCHW_NO) {
        return;
    }

    H264NalBits(sp, SEI_USER_DATA_UNREGISTERED, 8);
    COMMENT("last_payload_type_byte");

    while (cnt >= 255) {
        H264NalBits(sp, 0xFF, 0x8);
        COMMENT("ff_byte");
        cnt -= 255;
    }

    H264NalBits(sp, cnt, 8);
    COMMENT("last_payload_size_byte");

    /* Write uuid */
    for (i = 0; i < 16; i++) {
        H264NalBits(sp, pUserData[i], 8);
        COMMENT("uuid_iso_iec_11578_byte");
    }

    /* Write payload */
    for (i = 16; i < sei->userDataSize; i++) {
        H264NalBits(sp, pUserData[i], 8);
        COMMENT("user_data_payload_byte");
    }
}
