/*
*
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

#include <stdio.h>
#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_log.h"

#ifndef __H264D_RWFILE_H__
#define __H264D_RWFILE_H__


#define  FILE_NAME_SIZE       256


// input parameters from configuration file
typedef struct inp_par_t {
    RK_U32 iDecFrmNum;                         //!< set the max decode frame numbers
    char   infile_name[FILE_NAME_SIZE];        //!< H.264 input bitstrream
    char   cmp_path_dir[FILE_NAME_SIZE];       //!< golen
    char   cfgfile_name[FILE_NAME_SIZE];       //!< input configure file
    char   out_path_dir[FILE_NAME_SIZE];       //!< output
    //--- input file
    FILE   *fp_bitstream;
    FILE   *fp_yuv_data;
    //--- use in read bit stream
    RK_U32 raw_cfg;
    RK_U32 iFrmdecoded;
    RK_U8  is_fist_nalu;
    RK_U8  is_fist_frame;
    RK_U8  is_eof;
    RK_U8  is_new_frame;
    struct {
        RK_U8  *pbuf;
        RK_U32 offset;

        RK_U8  *pNALU;
        RK_U32 nalubytes;
        RK_U8  pfxbytes;  // start code prefix bytes
    } IO;
    struct {
        RK_U8  *pbuf;
        RK_U32  strmbytes;
    } strm;
    void    *bitctx;
} InputParams;

extern RK_U32 rkv_h264d_test_debug;



#define H264D_TEST_ERROR       (0x00000001)
#define H264D_TEST_ASSERT      (0x00000002)
#define H264D_TEST_WARNNING    (0x00000004)
#define H264D_TEST_TRACE       (0x00000008)

#define H264D_TEST_DUMPYUV     (0x00000010)



#define H264D_TEST_LOG(level, fmt, ...)\
do {\
    if (H264D_TEST_TRACE & rkv_h264d_test_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


#define ASSERT(val)\
do {\
    if (H264D_TEST_ASSERT & rkv_h264d_test_debug)\
        { mpp_assert(val); }\
} while (0)

#define FUN_CHECK(val)\
do{\
    if ((val) < 0) {\
        if (H264D_TEST_WARNNING & rkv_h264d_test_debug)\
            { mpp_log("Function error(%d).\n", __LINE__); }\
        goto __FAILED; \
}} while (0)


#define VAL_CHECK(ret, val, ...)\
do{\
    if (!(val)){ \
        ret = MPP_ERR_VALUE; \
        if (H264D_TEST_WARNNING & rkv_h264d_test_debug)\
            { mpp_log("value error(%d).\n", __LINE__); }\
        goto __FAILED; \
}} while (0)

#define MEM_CHECK(ret, val, ...)\
do{\
    if (!(val)) {\
        ret = MPP_ERR_MALLOC; \
        if (H264D_TEST_ERROR & rkv_h264d_test_debug)\
            { mpp_log("malloc buffer error(%d).\n", __LINE__); }\
        goto __FAILED; \
}} while (0)

#define FLE_CHECK(ret, val, ...)\
do{\
    if (!(val)) { \
        ret = MPP_ERR_OPEN_FILE; \
        if (H264D_TEST_WARNNING & rkv_h264d_test_debug)\
            { mpp_log("open file error(%d).\n", __LINE__); }\
        goto __FAILED; \
}} while (0)


#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h264d_configure  (InputParams *in, RK_S32 ac, char *av[]);
MPP_RET h264d_open_files (InputParams *in);
MPP_RET h264d_close_files(InputParams *in);
MPP_RET h264d_alloc_frame_buffer(InputParams *in);
MPP_RET h264d_read_one_frame    (InputParams *in);
MPP_RET h264d_free_frame_buffer (InputParams *in);

#ifdef __cplusplus
}
#endif

#endif /* __H264D_RWFILE_H__ */