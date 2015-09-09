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

#ifndef __H264D_PARSE_H__
#define __H264D_PARSE_H__
#include "rk_type.h"
#include "mpp_err.h"


typedef enum
{
	NALU_NULL = 0,

	//StreamError,
	//HaveNoStream,
	//NaluNotSupport,
	//ReadNaluError,
	//NALU_ERROR,
	//NALU_OK,
	//StartofNalu,
	//EndofStream,
	//ReallocBufError,
	//MidOfNalu,
	//EndOfNalu,
	//StartOfPicture,
	//StartOfSlice,
	//SkipNALU,
	//NALU_SPS,
	//NALU_SubSPS,
	//NALU_PPS,
	//NALU_SEI,

	NALU_MAX,
}NALU_STATUS;


typedef enum Rkv_slice_state_t
{
	SliceSTATE_NULL = 0,
	SliceSTATE_IDLE,

	//SliceSTATE_GetLastSlice,
	SliceSTATE_ResetSlice,
	//SliceSTATE_ReadNalu,
	//SliceSTATE_ParseNalu,
	//SliceSTATE_DecodeOneSlice,
	//SliceSTATE_InitPicture,
	//SliceSTATE_GetOuterDpbMemory,
	////SliceSTATE_InitSlice,
	//SliceSTATE_GetSliceData,
	//SliceSTATE_RegisterOneSlice,
	//SliceSTATE_RegisterOneFrame,
	//SliceSTATE_ExitPicture,
	//SliceSTATE_ErrorMaster,

	//SliceSTATE_Return,
	SliceSTATE_MAX,

}RKV_SLICE_STATUS;






#ifdef  __cplusplus
extern "C" {
#endif


#ifdef  __cplusplus
}
#endif


#endif /* __H264D_PARSE_H__ */
