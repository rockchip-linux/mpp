/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __MPP_DEC_H__
#define __MPP_DEC_H__

#include "rk_mpi.h"
#include "mpp_packet.h"
#include "mpp_buf_slot.h"

#define MPP_SYNTAX_MAX_NUMBER       8

/*
 * modified by mpp / hal
 *
 * index    : the index of current MppSyntax in structure array
 *            initialize at first and do not change it
 * status   : use for sync between hal and parser
 *
 *
 * modified by parser
 *
 * number   : the number of the data pointer array element
 * data     : the address of the pointer array, parser will add its data here
 */
typedef struct {
    RK_U32          index;
    RK_U32          status;

    RK_U32          number;
    void            **data;
} MppSyntax;

/*
 * slots    - all decoder need a slots interface to sync its internal dpb management
 *            with buffer group in mpp_dec
 *
 * the reset wait for extension
 */
typedef struct {
    MppBufSlots     slots;
} MppParserInitCfg;

typedef struct MppDecCtx_t MppDecCtx;

/*
 * MppDecParser is the data structure provided from different decoders
 *
 * They will be static register to mpp_dec for scaning
 * name     - decoder name
 * coding   - decoder coding type
 * ctx_size - decoder context size, mpp_dec will use this to malloc memory
 * flag     - reserve
 *
 * init     - decoder initialization function
 * deinit   - decoder de-initialization function
 * parse    - decoder main working function, mpp_dec will input packet and get output syntax
 * reset    - decoder reset function
 * flush    - decoder output all frames
 * control  - decoder configure function
 */
typedef struct {
    char            *name;
    MppCodingType   coding;
    RK_U32          ctx_size;
    RK_U32          flag;

    MPP_RET (*init)(void **ctx, MppParserInitCfg *cfg);
    MPP_RET (*deinit)(void *ctx);

    MPP_RET (*parse)(void *ctx, MppPacket pkt, MppSyntax *syn);

    MPP_RET (*reset)(void *ctx);
    MPP_RET (*flush)(void *ctx);
    MPP_RET (*control)(void *ctx, RK_S32 cmd, void *param);
} MppDecParser;

struct MppDecCtx_t {
    MppCodingType   coding;
    void            *ctx;
    MppSyntax       syntax[2];
    MppBufSlots     slots;
    MppDecParser    *parser;
};


#ifdef __cplusplus
extern "C" {
#endif

/*
 * main thread for all decoder. This thread will connect parser / hal / mpp
 */
void *mpp_dec_thread(void *data);

/*
 *
 */
MPP_RET mpp_dec_init(MppDecCtx **ctx, MppCodingType coding);
MPP_RET mpp_dec_deinit(MppDecCtx *ctx);

MPP_RET mpp_dec_parse(MppDecCtx *ctx, MppPacket pkt, MppSyntax **syntax);
MPP_RET mpp_dec_reset(MppDecCtx *ctx);
MPP_RET mpp_dec_flush(MppDecCtx *ctx);
MPP_RET mpp_dec_control(MppDecCtx *ctx, RK_S32 cmd, void *para);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_DEC_H__*/
