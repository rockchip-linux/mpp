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


#ifndef __AVSD_SYNTAX_H__
#define __AVSD_SYNTAX_H__

#include "rk_type.h"


//!< cavs decoder picture parameters structure
typedef struct _PicParams_Avsd {
    //!< sequence header
    RK_U32 profileId;
    RK_U32 levelId;
    RK_U32 progressiveSequence;
    RK_U32 horizontalSize;
    RK_U32 verticalSize;
    RK_U32 chromaFormat;
    RK_U32 aspectRatio;
    RK_U32 frameRateCode;
    RK_U32 bitRateValue;
    RK_U32 lowDelay;
    RK_U32 bbvBufferSize;
    //!< sequence display extension header
    RK_U32 videoFormat;
    RK_U32 sampleRange;
    RK_U32 colorDescription;
    RK_U32 colorPrimaries;
    RK_U32 transferCharacteristics;
    RK_U32 matrixCoefficients;
    RK_U32 displayHorizontalSize;
    RK_U32 displayVerticalSize;

    //!< picture header
    RK_U32 picCodingType;
    RK_U32 bbvDelay;
    RK_U16 bbvDelayExtension;
    RK_U8  timeCodeFlag;
    RK_U32 timeCode;

    RK_U32 pictureDistance;
    RK_U32 progressiveFrame;
    RK_U32 pictureStructure;
    RK_U32 advancedPredModeDisable;
    RK_U32 topFieldFirst;
    RK_U32 repeatFirstField;
    RK_U32 fixedPictureQp;
    RK_U32 pictureQp;
    RK_U32 pictureReferenceFlag;
    RK_U32 skipModeFlag;
    RK_U32 loopFilterDisable;
    RK_S32 alphaOffset;
    RK_S32 betaOffset;

    //!< weighting quant, AVS Plus stuff
    RK_U32 weightingQuantFlag;
    RK_U32 chromaQuantParamDisable;
    RK_S32 chromaQuantParamDeltaCb;
    RK_S32 chromaQuantParamDeltaCr;
    RK_U32 weightingQuantParamIndex;
    RK_U32 weightingQuantModel;
    RK_S32 weightingQuantParamDelta1[6];
    RK_S32 weightingQuantParamDelta2[6];
    RK_U32 weightingQuantParam[6]; // wqP[m][6]

    //!< advance entropy coding
    RK_U32 aecEnable;

    //!< picture enhance
    RK_U32 noForwardReferenceFlag;
    RK_U32 pbFieldEnhancedFlag;

} PicParams_Avsd, *LP_PicParams_Avsd;



typedef struct avsd_syntax_t {
    PicParams_Avsd     pp;
    RK_U8             *bitstream;
    RK_U32             bitstream_size;
} AvsdSyntax_t;



#endif /*__AVSD_SYNTAX_H__*/

