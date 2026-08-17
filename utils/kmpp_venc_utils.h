/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef KMPP_VENC_UTILS_H
#define KMPP_VENC_UTILS_H

#include "rk_venc_cmd.h"
#include "mpp_enc_frm_cfg.h"
#include "mpp_meta.h"
#include "kmpp_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* stack variable for a user data set with n trailing entries.
 * MppEncUserDataSetShm has a flexible data[] member, so back it with a
 * byte buffer in a union instead of nesting it inside another struct. */
#define MPP_ENC_UDS(name, n) \
    union { \
        rk_u8 buf[sizeof(MppEncUserDataSetShm) + sizeof(MppEncUserDataFullShm) * (n)]; \
        MppEncUserDataSetShm set; \
    } name

/* one / two trailing entries */
#define MPP_ENC_UDS1(name)  MPP_ENC_UDS(name, 1)
#define MPP_ENC_UDS2(name)  MPP_ENC_UDS(name, 2)

/* Standard test UUID for unregistered user data SEI */
extern RK_U8 venc_test_uuid[16];

/* Deep-copy a frame cfg entry with VLA tails and bind test userdata. */
MppEncFrmCfg *venc_dup_frm_cfg_with_ud(const MppEncFrmCfg *entry, RK_U8 *ud_buf, RK_U32 ud_buf_size);

/* Set USER_DATA test pattern into frame meta (single buffer variant). */
MPP_RET kmpp_venc_gen_userdata(KmppMeta meta, RK_U8 *ud_buf, RK_U32 ud_buf_size);

/* Set USER_DATAS into frame meta (uuid + data). */
MPP_RET kmpp_venc_gen_userdatas(KmppMeta meta, const RK_U8 *uuid,
                                RK_U8 *ud_buf, RK_U32 ud_buf_size);

/*
 * Set ROI regions from stable config — converts MppEncFrmRoi
 * (Q16 ratio coordinates) to MppEncROICfgLegacy pixel coordinates.
 * w/h are the frame dimensions, used for ratio-to-pixel conversion.
 */
MPP_RET kmpp_venc_gen_roi(KmppMeta meta, RK_U32 w, RK_U32 h,
                          RK_U32 roi_cnt, const MppEncFrmRoi *roi);

/*
 * Set OSD regions from stable config — converts MppEncFrmOsd
 * (Q16 ratio coordinates) to MppEncOSDData3 pixel coordinates.
 * w/h are the frame dimensions, used for ratio-to-pixel conversion.
 * stride == 0 is auto-derived from the region width and format.
 */
MPP_RET kmpp_venc_gen_osd(KmppMeta meta, RK_U32 w, RK_U32 h,
                          RK_U32 osd_cnt, const MppEncFrmOsd *osd);

/*
 * Apply a single frame config entry to KMPP frame meta.
 * Dispatches to scalar, userdata/userdatas, ROI, OSD and JPEG ROI
 * w/h are the frame dimensions, used for ratio-to-pixel conversion.
 */
MPP_RET kmpp_venc_gen_frame_meta(KmppMeta meta, RK_U32 w, RK_U32 h, const MppEncFrmCfg *entry);

/*
 * Scan H.264/H.265 SEI NAL for a userdata unregistered payload with an
 * exact UUID and exact-length payload match.
 * Returns 1 on match, 0 otherwise.
 */
RK_S32 kmpp_venc_scan_sei_userdata(const RK_U8 *data, RK_S32 len,
                                   const RK_U8 *uuid, const void *expect, RK_S32 expect_len);

/*
 * Persistent test resources referenced by ordinary MppMeta.
 * Zero-initialize before first use and release with
 * mpp_venc_frm_meta_deinit after the last frame is consumed.
 */
typedef struct MppEncFrmMetaData_t {
    RK_U8               *ud_buf;
    RK_U32              ud_buf_size;
    MppEncROICfg        roi;
    MppEncROIRegion     roi_regions[8];
    MppJpegROICfg       jpeg_roi;
    MppEncOSDData3      osd;
    KmppBuffer          osd_buffers[8];
} MppEncFrmMetaData;

/* Set USER_DATA into ordinary MppMeta. The payload is deep-copied by MppMeta. */
MPP_RET mpp_venc_gen_userdata(MppMeta meta, RK_U8 *ud_buf, RK_U32 ud_buf_size);

/* Set USER_DATAS (uuid + data) into ordinary MppMeta with a deep copy. */
MPP_RET mpp_venc_gen_userdatas(MppMeta meta, const RK_U8 *uuid,
                               RK_U8 *ud_buf, RK_U32 ud_buf_size);

/*
 * Set ROI regions into ordinary MppMeta after converting Q16 ratios to
 * pixel coordinates. data owns the object referenced by MppMeta.
 */
MPP_RET mpp_venc_gen_roi(MppMeta meta, RK_U32 w, RK_U32 h,
                         RK_U32 roi_cnt, const MppEncFrmRoi *roi,
                         MppEncFrmMetaData *data);

/*
 * Set OSD regions into ordinary MppMeta and create test pixel buffers.
 * data owns the objects and buffers referenced by MppMeta.
 */
MPP_RET mpp_venc_gen_osd(MppMeta meta, RK_U32 w, RK_U32 h,
                         RK_U32 osd_cnt, const MppEncFrmOsd *osd,
                         MppEncFrmMetaData *data);

/*
 * Set JPEG ROI regions into ordinary MppMeta after converting Q16 ratios
 * to pixel coordinates. data owns the object referenced by MppMeta.
 */
MPP_RET mpp_venc_gen_jpeg_roi(MppMeta meta, RK_U32 w, RK_U32 h,
                              RK_U32 jpeg_roi_cnt,
                              const MppEncFrmJpegRoi *jpeg_roi,
                              RK_U32 non_roi_level, RK_U32 non_roi_en,
                              MppEncFrmMetaData *data);

/* Apply one frame config entry to ordinary MppMeta. */
MPP_RET mpp_venc_gen_frame_meta(MppMeta meta, RK_U32 w, RK_U32 h,
                                const MppEncFrmCfg *entry,
                                MppEncFrmMetaData *data);

/* Release buffers owned by MppEncFrmMetaData. */
void mpp_venc_frm_meta_deinit(MppEncFrmMetaData *data);

#ifdef __cplusplus
}
#endif

#endif /* KMPP_VENC_UTILS_H */
