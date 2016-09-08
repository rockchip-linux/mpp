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

#ifndef __DXVA_SYNTAX_H__
#define __DXVA_SYNTAX_H__

#include "rk_type.h"

enum __MIDL___MIDL_itf_dxva2api_0000_0000_0012 {
    DXVA2_PictureParametersBufferType         = 0,
    DXVA2_MacroBlockControlBufferType         = 1,
    DXVA2_ResidualDifferenceBufferType        = 2,
    DXVA2_DeblockingControlBufferType         = 3,
    DXVA2_InverseQuantizationMatrixBufferType = 4,
    DXVA2_SliceControlBufferType              = 5,
    DXVA2_BitStreamDateBufferType             = 6,
    DXVA2_MotionVectorBuffer                  = 7,
    DXVA2_FilmGrainBuffer                     = 8
};

typedef struct _DXVA2_ConfigPictureDecode {
    //GUID guidConfigBitstreamEncryption;
    //GUID guidConfigMBcontrolEncryption;
    //GUID guidConfigResidDiffEncryption;
    RK_U32 ConfigBitstreamRaw;
    //UINT ConfigMBcontrolRasterOrder;
    //UINT ConfigResidDiffHost;
    //UINT ConfigSpatialResid8;
    //UINT ConfigResid8Subtraction;
    //UINT ConfigSpatialHost8or9Clipping;
    //UINT ConfigSpatialResidInterleaved;
    //UINT ConfigIntraResidUnsigned;
    //UINT ConfigResidDiffAccelerator;
    //UINT ConfigHostInverseScan;
    //UINT ConfigSpecificIDCT;
    //UINT Config4GroupedCoefs;
    //USHORT ConfigMinRenderTargetBuffCount;
    //USHORT ConfigDecoderSpecific;
} DXVA2_ConfigPictureDecode;

typedef struct _DXVA2_DecodeBufferDesc {
    RK_U32 CompressedBufferType;
    RK_U32 BufferIndex;
    RK_U32 DataOffset;
    RK_U32 DataSize;
    RK_U32 FirstMBaddress;
    RK_U32 NumMBsInBuffer;
    RK_U32 Width;
    RK_U32 Height;
    RK_U32 Stride;
    RK_U32 ReservedBits;
    void   *pvPVPState;
}   DXVA2_DecodeBufferDesc;

#endif /*__DXVA_SYNTAX_H__*/

