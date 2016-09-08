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

#ifndef __AVSD_IMPL_H__
#define __AVSD_IMPL_H__


#include "rk_type.h"

#include "mpp_err.h"
#include "mpp_packet.h"

#include "hal_task.h"



#ifdef  __cplusplus
extern "C" {
#endif

void  avsd_test_main(int argc, char **argv);

void *lib_avsd_create();
int   lib_avsd_init(void *decoder);
int   lib_avsd_destory(void *decoder);
int   lib_avsd_prepare(void *decoder, unsigned char *buf, int len);
int   lib_avsd_decode_one_frame(void *decoder, int *got_frame);
int   lib_avsd_get_outframe(void *decoder, int *w, int *h, unsigned char *data[], int *stride, int *crop);


#ifdef  __cplusplus
}
#endif

#endif /*__AVSD_IMPL_H__*/
