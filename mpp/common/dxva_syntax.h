/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef DXVA_SYNTAX_H
#define DXVA_SYNTAX_H

#include "rk_type.h"

enum MIDL___MIDL_itf_dxva2_e {
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

typedef struct DXVA2_ConfigPictureDecode_t {
    //GUID guidConfigBitstreamEncryption;
    //GUID guidConfigMBcontrolEncryption;
    //GUID guidConfigResidDiffEncryption;
    RK_U32 ConfigBitstreamRaw;
    //RK_U32 ConfigMBcontrolRasterOrder;
    //RK_U32 ConfigResidDiffHost;
    //RK_U32 ConfigSpatialResid8;
    //RK_U32 ConfigResid8Subtraction;
    //RK_U32 ConfigSpatialHost8or9Clipping;
    //RK_U32 ConfigSpatialResidInterleaved;
    //RK_U32 ConfigIntraResidUnsigned;
    //RK_U32 ConfigResidDiffAccelerator;
    //RK_U32 ConfigHostInverseScan;
    //RK_U32 ConfigSpecificIDCT;
    //RK_U32 Config4GroupedCoefs;
    //RK_U16 ConfigMinRenderTargetBuffCount;
    //RK_U16 ConfigDecoderSpecific;
} DXVA2_ConfigPictureDecode;

typedef struct DXVA2_DecodeBufferDesc_t {
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
} DXVA2_DecodeBufferDesc;

#endif /* DXVA_SYNTAX_H */

