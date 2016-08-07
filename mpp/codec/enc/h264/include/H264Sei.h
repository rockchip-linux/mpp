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

#ifndef H264_SEI_H
#define H264_SEI_H

#include "basetype.h"
#include "H264PutBits.h"

typedef struct {
    RK_U32 fts;    /* Full time stamp */
    RK_U32 timeScale;
    RK_U32 nuit;   /* number of units in tick */
    RK_U32 time;   /* Modulo time */
    RK_U32 secf;
    RK_U32 sec;    /* Seconds */
    RK_U32 minf;
    RK_U32 min;    /* Minutes */
    RK_U32 hrf;
    RK_U32 hr; /* Hours */
} timeStamp_s;

typedef struct {
    timeStamp_s ts;
    RK_U32 nalUnitSize;
    RK_U32 enabled;
    true_e byteStream;
    RK_U32 hrd;    /* HRD conformance */
    RK_U32 seqId;
    RK_U32 icrd;   /* initial cpb removal delay */
    RK_U32 icrdLen;
    RK_U32 icrdo;  /* initial cpb removal delay offset */
    RK_U32 icrdoLen;
    RK_U32 crd;    /* CPB removal delay */
    RK_U32 crdLen;
    RK_U32 dod;    /* DPB removal delay */
    RK_U32 dodLen;
    RK_U32 psp;
    RK_U32 ps;
    RK_U32 cts;
    RK_U32 cntType;
    RK_U32 cdf;
    RK_U32 nframes;
    RK_U32 toffs;
    RK_U32 toffsLen;
    RK_U32 userDataEnabled;
    const RK_U8 * pUserData;
    RK_U32 userDataSize;
} sei_s;

void H264InitSei(sei_s * sei, true_e byteStream, RK_U32 hrd, RK_U32 timeScale,
                 RK_U32 nuit);
void H264UpdateSeiTS(sei_s * sei, RK_U32 timeInc);
void H264FillerSei(stream_s * sp, sei_s * sei, RK_S32 cnt);
void H264BufferingSei(stream_s * stream, sei_s * sei);
void H264PicTimingSei(stream_s * stream, sei_s * sei);
void H264UserDataUnregSei(stream_s * sp, sei_s * sei);

#endif
