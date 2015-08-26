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

#ifndef __MPP_H__
#define __MPP_H__

#include "rk_mpi.h"
#include "mpp_list.h"
#include "mpp_thread.h"

class Mpp {
public:
    Mpp(MppCtxType type);
    ~Mpp();

    mpp_list        *packets;
    mpp_list        *frames;

    pthread_t       thread_codec;
    pthread_t       thread_hal;
	RK_S32          thread_codec_running;
	RK_S32          thread_codec_reset;

    RK_U32          status;

    MPP_RET put_packet(MppPacket packet);
    MPP_RET get_frame(MppFrame *frame);

    MPP_RET put_frame(MppFrame frame);
    MPP_RET get_packet(MppPacket *packet);

private:
    void thread_start(MppThreadFunc func);
    void thread_stop();

    Mpp();
    Mpp(const Mpp &);
    Mpp &operator=(const Mpp &);
};

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif /*__MPP_H__*/
