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

#ifndef __M2VD_COM_H__
#define __M2VD_COM_H__

#include <stdio.h>
#include <stdlib.h>

#include "rk_type.h"
#include "mpp_log.h"

#define M2VD_DEMO_MODE         0

//------------------------ temp ---------------------------------
#define M2VD_TEST_TRACE       (0x00000001)
#define M2VD_TEST_TIME        (0x00000002)
#define M2VD_TEST_MUTI_THREAD (0x00000004)
#define M2VD_TEST_DUMPYUV     (0x00000008)
#define M2VD_TEST_FPGA        (0x00000010)

//log of diff
#define M2VD_DBG_FUNCTION          (0x00000001)
#define M2VD_DBG_ASSERT            (0x00000002)
#define M2VD_DBG_WARNNING          (0x00000004)
#define M2VD_DBG_LOG               (0x00000008)
#define M2VD_DBG_SEC_HEADER        (0x00000010)
#define M2VD_DBG_DUMP_REG          (0x00000020)




extern RK_U32 m2vd_debug;

#define M2VD_ERR(fmt, ...)\
do {\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define M2VD_ASSERT(val)\
do {\
    if (M2VD_DBG_ASSERT & m2vd_debug)\
        { mpp_assert(val); }\
} while (0)

#define M2VD_WARNNING(fmt, ...)\
do {\
    if (M2VD_DBG_WARNNING & m2vd_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define M2VD_LOG(fmt, ...)\
do {\
    if (M2VD_DBG_LOG & m2vd_debug)\
        {  mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


//check function return
#define M2VD_CHK_F(val)                                            \
    do{                                                       \
        if((val) < 0) {                                       \
            ret = (val);                                      \
            M2VD_WARNNING("func return error(L%d), ret:%d\n", __LINE__, ret); \
            goto __FAILED;                                    \
        }                                                     \
      } while (0)

//check value if is zero or NULL
#define M2VD_CHK_V(val, ...)\
    do{ if(!(val)){\
    ret = MPP_ERR_VALUE;\
    M2VD_WARNNING("value error(L%d), val:%d\n", __LINE__, val);\
    goto __FAILED;\
    } } while (0)

//memory malloc check
#define M2VD_CHK_M(val, ...)\
    do{ if(!(val)) {\
    ret = MPP_ERR_MALLOC;\
    M2VD_ERR("malloc buffer error(%d), pointer:%p\n", __LINE__, val);\
    M2VD_ASSERT(0); goto __FAILED;\
    } } while (0)

//file check
#define M2VD_CHK_FILE(val, path, ...)\
    do{ if(!(val)) {\
            ret = MPP_ERR_OPEN_FILE;\
            M2VD_WARNNING("open file error(line%d): %s\n", __LINE__, path);\
            M2VD_ASSERT(0); goto __FAILED;\
    } } while (0)

//!< input check
#define M2VD_CHK_I(val, ...)\
    do{ if(!(val)) {\
    ret = MPP_ERR_INIT;\
    M2VD_WARNNING("input empty(%d), val:%d\n", __LINE__, val);\
    goto __FAILED;\
    } } while (0)

#define m2vd_dbg_func(tag) \
    do {\
        if (M2VD_DBG_FUNCTION & m2vd_debug)\
            { mpp_log("%s: line(%d), func(%s)", tag, __LINE__, __FUNCTION__); }\
    } while (0)




#define  M2VD_FCLOSE(fp)    do{ if(fp) fclose(fp); fp = NULL; } while (0)

#endif
