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
    u32 fts;    /* Full time stamp */
    u32 timeScale;
    u32 nuit;   /* number of units in tick */
    u32 time;   /* Modulo time */
    u32 secf;
    u32 sec;    /* Seconds */
    u32 minf;
    u32 min;    /* Minutes */
    u32 hrf;
    u32 hr; /* Hours */
} timeStamp_s;

typedef struct {
    timeStamp_s ts;
    u32 nalUnitSize;
    u32 enabled;
    true_e byteStream;
    u32 hrd;    /* HRD conformance */
    u32 seqId;
    u32 icrd;   /* initial cpb removal delay */
    u32 icrdLen;
    u32 icrdo;  /* initial cpb removal delay offset */
    u32 icrdoLen;
    u32 crd;    /* CPB removal delay */
    u32 crdLen;
    u32 dod;    /* DPB removal delay */
    u32 dodLen;
    u32 psp;
    u32 ps;
    u32 cts;
    u32 cntType;
    u32 cdf;
    u32 nframes;
    u32 toffs;
    u32 toffsLen;
    u32 userDataEnabled;
    const u8 * pUserData;
    u32 userDataSize;
} sei_s;

void H264InitSei(sei_s * sei, true_e byteStream, u32 hrd, u32 timeScale,
                 u32 nuit);
void H264UpdateSeiTS(sei_s * sei, u32 timeInc);
void H264FillerSei(stream_s * sp, sei_s * sei, i32 cnt);
void H264BufferingSei(stream_s * stream, sei_s * sei);
void H264PicTimingSei(stream_s * stream, sei_s * sei);
void H264UserDataUnregSei(stream_s * sp, sei_s * sei);

#endif
