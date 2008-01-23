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


typedef unsigned long       DWORD;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       ULONG;
typedef unsigned short      USHORT;
typedef unsigned char       UCHAR;
typedef unsigned int        UINT;

typedef signed   int        BOOL;
typedef signed   int        INT;
typedef signed   char       CHAR;
typedef signed   short      SHORT;
typedef signed   long       LONG;
typedef void               *PVOID;


enum __MIDL___MIDL_itf_dxva2api_0000_0000_0012
{
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

typedef struct _DXVA2_ConfigPictureDecode
{
    //GUID guidConfigBitstreamEncryption;
    //GUID guidConfigMBcontrolEncryption;
    //GUID guidConfigResidDiffEncryption;
    UINT ConfigBitstreamRaw;
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
}DXVA2_ConfigPictureDecode;

typedef struct _DXVA2_DecodeBufferDesc
{
    DWORD CompressedBufferType;
    UINT BufferIndex;
    UINT DataOffset;
    UINT DataSize;
    UINT FirstMBaddress;
    UINT NumMBsInBuffer;
    UINT Width;
    UINT Height;
    UINT Stride;
    UINT ReservedBits;
    PVOID pvPVPState;
}   DXVA2_DecodeBufferDesc;

#endif /*__DXVA_SYNTAX_H__*/

