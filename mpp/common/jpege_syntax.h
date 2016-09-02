#ifndef __JPEGE_SYNTAX_H__
#define __JPEGE_SYNTAX_H__

#include "rk_type.h"





typedef struct jpege_syntax_t {
	RK_U32 qp;
	RK_U32 qpMin;
	RK_U32 qpMax;
	RK_U32 xFill;
	RK_U32 yFill;
	RK_U32 frameNum;
	RK_U32 idrPicId;
	RK_U32 mbsInRow;
	RK_U32 mbsInCol;
	RK_U32 jpegMode;
	RK_U32 cabacInitIdc;
	RK_U32 enableCabac;
	RK_U32 sliceSizeMbRows;
	RK_U32 outputStrmBase;
	RK_U32 outputStrmSize;
	RK_U32 inputLumBase;             // inputLumBase
	RK_U32 inputCrBase;               // inputCbBase
	RK_U32 inputCbBase;               // inputCrBase
	RK_U32 pixelsOnRow;          // inputImageFormat
	RK_U32 jpegSliceEnable;
	RK_U32 jpegRestartInterval;
	RK_U32 inputLumaBaseOffset;
	RK_U32 filterDisable;
	RK_U32 inputChromaBaseOffset;
	RK_S32 chromaQpIndexOffset;
	RK_U32 strmStartMSB;
	RK_U32 strmStartLSB;	
	RK_S32 madQpDelta;
	RK_U32 firstFreeBit;
	RK_U32 madThreshold;
	RK_S32 sliceAlphaOffset;
	RK_S32 sliceBetaOffset;
	RK_U32 transform8x8Mode;
	RK_U32 jpegcolor_conversion_coeff_a;    //colorConversionCoeffA
	RK_U32 jpegcolor_conversion_coeff_b;    //colorConversionCoeffB
	RK_U32 jpegcolor_conversion_coeff_c;    //colorConversionCoeffC
	RK_U32 jpegcolor_conversion_coeff_e;    //colorConversionCoeffE
	RK_U32 jpegcolor_conversion_coeff_f;    //colorConversionCoeffF
	RK_U32 jpegcolor_conversion_r_mask_msb; //rMaskMsb
	RK_U32 jpegcolor_conversion_g_mask_msb; //gMaskMsb
	RK_U32 jpegcolor_conversion_b_mask_msb; //bMaskMsb

	/* RKVENC extra syntax below */
	RK_S32 profile_idc; //TODO: may be removed later, get from sps/pps instead
	RK_S32 level_idc; //TODO: may be removed later, get from sps/pps instead
	RK_S32 link_table_en;
	RK_S32 keyframe_max_interval;
} jpege_syntax; //EncJpegInstance.h





#endif
