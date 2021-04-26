/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#include "iep2_pd.h"

#include <stdio.h>
#ifdef SIMULATE
#include <malloc.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "mpp_common.h"
#include "mpp_log.h"
#include "iep_common.h"

#include "iep2.h"
#include "iep2_api.h"

#define RKMIN(a, b)             (((a) < (b)) ? (a) : (b))
#define RKMAX(a, b)             (((a) > (b)) ? (a) : (b))

#define PD_TS   1
#define PD_BS   2
#define PD_DF   0
#define PD_ID   3

static int pd_table[][5] = {
    { PD_TS, PD_DF, PD_BS, PD_DF, PD_DF },
    { PD_DF, PD_BS, PD_DF, PD_TS, PD_DF },
    { PD_TS, PD_BS, PD_DF, PD_DF, PD_DF },
    { PD_BS, PD_DF, PD_DF, PD_TS, PD_DF },
    { PD_DF, PD_DF, PD_DF, PD_DF, PD_DF }
};

static int sp_table[][5] = {
    { 0, 1, 1, 0, 0 },
    { 0, 0, 1, 1, 0 },
    { 0, 1, 0, 0, 0 },
    { 0, 1, 1, 1, 0 },
    { 1, 1, 1, 1, 1 }
};

static int fp_table[][5] = {
    { 1, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 0 },
    { 0, 1, 1, 0, 0 },
    { 1, 1, 1, 1, 0 },
    { 1, 1, 1, 1, 1 }
};

static char* pd_titles[] = {
    "PULLDOWN 3:2:3:2",
    "PULLDOWN 2:3:2:3",
    "PULLDOWN 2:3:3:2",
    "PULLDOWN 3:2:2:3",
    "PULLDOWN UNKNOWN"
};

void iep2_check_pd(struct iep2_api_ctx *ctx)
{
    struct iep2_pd_info *pd_inf = &ctx->pd_inf;
    int tcnt = ctx->output.dect_pd_tcnt;
    int bcnt = ctx->output.dect_pd_bcnt;
    int tdiff = ctx->output.ff_gradt_tcnt + 1;
    int bdiff = ctx->output.ff_gradt_bcnt + 1;
    int idx = pd_inf->i % 5;
    int ff00t = (ctx->output.dect_ff_cur_tcnt << 5) / tdiff;
    int ff00b = (ctx->output.dect_ff_cur_bcnt << 5) / bdiff;
    int nz = ctx->output.dect_ff_nz + 1;
    int f = ctx->output.dect_ff_comb_f;
    size_t i, j;

    pd_inf->spatial[idx] = RKMIN(ff00t, ff00b);
    pd_inf->temporal[idx] = (tcnt < 32) | ((bcnt < 32) << 1);
    pd_inf->fcoeff[idx] = f * 100 / nz;

    iep_dbg_trace("pd tcnt %d bcnt %d\n", tcnt, bcnt);
    iep_dbg_trace("temporal(%d, %d) %d %d %d %d %d\n",
                  idx, pd_inf->step,
                  pd_inf->temporal[0],
                  pd_inf->temporal[1],
                  pd_inf->temporal[2],
                  pd_inf->temporal[3],
                  pd_inf->temporal[4]);
    iep_dbg_trace("spatial(%d, %d) %d %d %d %d %d\n",
                  idx, pd_inf->step,
                  pd_inf->spatial[0],
                  pd_inf->spatial[1],
                  pd_inf->spatial[2],
                  pd_inf->spatial[3],
                  pd_inf->spatial[4]);
    iep_dbg_trace("fcoeff(%d, %d) %d %d %d %d %d\n",
                  idx, pd_inf->step,
                  pd_inf->fcoeff[0],
                  pd_inf->fcoeff[1],
                  pd_inf->fcoeff[2],
                  pd_inf->fcoeff[3],
                  pd_inf->fcoeff[4]);

    if (pd_inf->pdtype != PD_TYPES_UNKNOWN && pd_inf->step != -1) {
        int n = (int)pd_inf->pdtype;
        int type = pd_table[n][(pd_inf->step + 1) % 5];

        if ((type == PD_TS && !(tcnt < 32)) ||
            (type == PD_BS && !(bcnt < 32))) {
            pd_inf->pdtype = PD_TYPES_UNKNOWN;
            pd_inf->step = -1;
        }
    }

    pd_inf->step = pd_inf->step != -1 ? (pd_inf->step + 1) % 5 : -1;

    if (pd_inf->pdtype != PD_TYPES_UNKNOWN) {
        pd_inf->i++;
        return;
    } else {
        iep_dbg_trace("pulldown recheck start:\n");
    }

    for (i = 0; i < MPP_ARRAY_ELEMS(pd_table); ++i) {
        if (pd_inf->temporal[idx] == pd_table[i][0] &&
            pd_inf->temporal[(idx + 1) % 5] == pd_table[i][1] &&
            pd_inf->temporal[(idx + 2) % 5] == pd_table[i][2] &&
            pd_inf->temporal[(idx + 3) % 5] == pd_table[i][3] &&
            pd_inf->temporal[(idx + 4) % 5] == pd_table[i][4]) {

            if (i != PD_TYPES_UNKNOWN) {
                int vmax = 0x7fffffff;
                int vmin = 0;
                int fmax = 0x7fffffff;
                int fmin = 0;

                iep_dbg_trace("get pulldown type %s\n", pd_titles[i]);

                for (j = 0; j < MPP_ARRAY_ELEMS(sp_table[i]); ++j) {
                    if (sp_table[i][j] == 1) {
                        vmax = RKMIN(vmax, pd_inf->spatial[j]);
                    } else {
                        vmin = RKMAX(vmin, pd_inf->spatial[j]);
                    }
                }

                for (j = 0; j < MPP_ARRAY_ELEMS(fp_table[i]); ++j) {
                    if (fp_table[i][j] == 1) {
                        fmax = RKMIN(fmax, pd_inf->fcoeff[(idx + j) % 5]);
                    } else {
                        fmin = RKMAX(fmin, pd_inf->fcoeff[(idx + j) % 5]);
                    }
                }

                if (vmax > vmin || fmax > fmin) {
                    mpp_log("confirm pulldown type %s\n", pd_titles[i]);
                    pd_inf->pdtype = i;
                    pd_inf->step = 0;
                }
            }
            break;
        }
    }

    pd_inf->i++;
}

char *PD_COMP_STRING[] = {
    "PD_COMP_CC",
    "PD_COMP_CN",
    "PD_COMP_NC",
    "PD_COMP_NON"
};

int iep2_pd_get_output(struct iep2_pd_info *pd_inf)
{
    int flag = PD_COMP_FLAG_CC;
    int step = (pd_inf->step + 1) % 5;

    switch (pd_inf->pdtype) {
    case PD_TYPES_3_2_3_2:
        switch (step) {
        case 1:
            flag = PD_COMP_FLAG_NC;
            break;
        case 2:
            flag = PD_COMP_FLAG_NON;
            break;
        }
        break;
    case PD_TYPES_2_3_2_3:
        switch (step) {
        case 2:
            flag = PD_COMP_FLAG_CN;
            break;
        case 3:
            flag = PD_COMP_FLAG_NON;
            break;
        }
        break;
    case PD_TYPES_2_3_3_2:
        switch (step) {
        case 2:
            flag = PD_COMP_FLAG_NON;
            break;
        }
        break;
    case PD_TYPES_3_2_2_3:
        switch (step) {
        case 1:
        case 2:
            flag = PD_COMP_FLAG_CN;
            break;
        case 3:
            flag = PD_COMP_FLAG_NON;
            break;
        }
        break;
    default:
        mpp_log("unsupport telecine format %s\n",
                pd_titles[(int)pd_inf->pdtype]);
        return -1;
    }

    iep_dbg_trace("-------------------------------------------------\n");
    iep_dbg_trace("step %d, idx %d, flag %s\n",
                  pd_inf->step, pd_inf->i, PD_COMP_STRING[flag]);

    return flag;
}
