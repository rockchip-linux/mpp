/*
 *
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

#ifndef __MPP_INFO_H__
#define __MPP_INFO_H__

typedef enum RK_CHIP_TYPE {
    NONE,
    RK29,
    RK30,
    RK31,

    RK_CHIP_NUM = 0x100,
} RK_CHIP_TYPE;

#ifdef __cplusplus
class mpp_info
{
public:
    RK_CHIP_TYPE get_chip_type() {return chip_type;}
    int  get_mpp_revision() {return mpp_revision;}
    void show_mpp_info();

    static mpp_info *getInstance();
    virtual ~mpp_info() {};
private:
    static mpp_info *singleton;
    mpp_info();
    int      mpp_revision;
    char    *mpp_info_revision;
    char    *mpp_info_author;
    char    *mpp_info_date;
    RK_CHIP_TYPE chip_type;
};

extern "C" {
#endif /* __cplusplus */

RK_CHIP_TYPE get_chip_type();

#ifdef __cplusplus
}
#endif

#endif /*__MPP_INFO_H__*/
