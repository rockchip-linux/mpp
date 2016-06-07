/*
 *
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define MODULE_TAG "vpu"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "mpp_log.h"

#include "vpu.h"

#define VPU_IOC_MAGIC                       'l'
#define VPU_IOC_SET_CLIENT_TYPE             _IOW(VPU_IOC_MAGIC, 1, unsigned long)
#define VPU_IOC_GET_HW_FUSE_STATUS          _IOW(VPU_IOC_MAGIC, 2, unsigned long)
#define VPU_IOC_SET_REG                     _IOW(VPU_IOC_MAGIC, 3, unsigned long)
#define VPU_IOC_GET_REG                     _IOW(VPU_IOC_MAGIC, 4, unsigned long)
#define VPU_IOC_PROBE_IOMMU_STATUS          _IOR(VPU_IOC_MAGIC, 5, unsigned long)

typedef struct VPUReq {
    RK_U32 *req;
    RK_U32  size;
} VPUReq_t;

static int vpu_service_status = -1;
#define VPU_SERVICE_TEST    \
    do { \
        if (vpu_service_status < 0) { \
            vpu_service_status = (access("/dev/vpu_service", F_OK) == 0); \
        } \
    } while (0)

#define VPU_DBG_RKV    (0x000000001)

static RK_U32 vpu_client_debug = 0;
#define VPU_DBG(level, fmt, ...)\
     do {\
         if (level & vpu_client_debug)\
            { mpp_log(fmt, ## __VA_ARGS__); }\
     } while (0)

#define GetBitVal(val, pos)   ( ( (val)>>(pos) ) & 0x1 & (val) )

const char *g_sel_strname[46] = 
{
	" ",
	"cabac in buffer empty",
	"cabac in buffer full",
	"cabac out buffer empty",
	"cabac out buffer full",
	"transd input data ready",
	"transd write data to recon allow",
	"dec2transd cmd empty",
	"dec2transd cmd full",
	"transd2dblk bs fifo empty",
	"transd2dblk bs fifo full",
	"dec2intra cmd fifo empty",
	"dec2intra cmd fifo full",
	"mc2recon cmd fifo empty",
	"mc2recon cmd fifo full",
	"mc2recon data fifo empty",
	"mc2recon data fifo full",
	"recon2filter data write allow",
	"inter2busifd cmd fifo empty",
	"inter2busifd cmd fifo full",
	"busifd2mc data fifo empty",
	"busifd2mc data fifo full",
	"bus working status",
	"dec2inter cmd fifo empty",
	"dec2inter cmd fifo full",
	"inter2mc cmd fifo empty",
	"inter2mc cmd fifo full",
	"inter2dblk bs fifo empty",
	"inter2dblk bs fifo full",
	"colmv_rbuf_empty",
	"colmv_rbuf_full",
	"colmv_wbuf_empty",
	"colmv_wbuf_da_full",
	"dblk input data valid",
	"dblk can't write data to sao",
	"dec2loopfilter cmd fifo empty",
	"dec2loopfilter cmd fifo full",
	"sao input data valid",
	"busifd hold back sao write data",
	"sao output data valid",
	"dec_ctrl read cmd_num",
	"aver_cycle",
	"aver_rdata",
	"aver_wdata",
	"stream_len"
};

typedef struct vpu_debug_ctx_t
{
	FILE *fp_log;
	RK_U64 choice_sel;
	VPU_CLIENT_TYPE type;
	RK_U32 cycles[43];
	RK_U32 total_cycles[43];
	RK_U32 ddr_rdata[43];
	RK_U32 ddr_wdata[43];
}VpuDebugCtx_t;

typedef struct swreg68_performance_sel_t {
	RK_U32 perf_cnt0_sel : 6;
	RK_U32 reserve0 : 2;
	RK_U32 perf_cnt1_sel : 6;
	RK_U32 reserve1 : 2;
	RK_U32 perf_cnt2_sel : 6;
} Sw68_PerSel_t;

static VpuDebugCtx_t g_vpu_debug_ctx;

static void vpu_log_deinit(VpuDebugCtx_t *p)
{
	if (p->fp_log) {
		fclose(p->fp_log);
		p->fp_log = NULL;
	}
}

static int vpu_log_init(VPU_CLIENT_TYPE type, VpuDebugCtx_t *p)
{
	memset(p, 0, sizeof(*p));
	mpp_env_get_u32("vpu_client_debug", &vpu_client_debug, 0);
	if ((vpu_client_debug & VPU_DBG_RKV) && (type == VPU_DEC_RKV)){
		RK_U32 choice_sel0 = 0;
		RK_U32 choice_sel1 = 0;
		mpp_env_get_u32("vpu_client_debug_choice0", &choice_sel0, 0xFFFFFFFF);
		mpp_env_get_u32("vpu_client_debug_choice1", &choice_sel1, 0x0000FFFF);
		p->choice_sel = (choice_sel1 << 32) | choice_sel0;
		mpp_log_f("choice_sel0=%08x, choice_sel1=%08x, p->choice_sel=%08x \n", choice_sel0, choice_sel1, p->choice_sel);
		p->type = VPU_DEC_RKV;
		p->fp_log = fopen("/mnt/usb_storage/USB_DISK1/vpu_client_rkv_debug.csv", "w+b");
		if (p->fp_log) {
			RK_U32 i = 0;
			for (i = 1; i <= 44; i++) {
				if (1/*GetBitVal(p->choice_sel, i)*/) {
					fprintf(p->fp_log, "%s, ", g_sel_strname[i]);
				}				
			}
			fprintf(p->fp_log, "\n");
		}
	}
	return 0;
}

RK_S32 vpu_client_performance(int socket, RK_U32 *regs, RK_U32 nregs, VpuDebugCtx_t *p)
{
	RK_S32 ret = 0;
	if ((vpu_client_debug & VPU_DBG_RKV) && (p->type == VPU_DEC_RKV)) {

		RK_U32 i = 0;

		for (i = 1; i <= 42; i += 3) {
			RK_U32 m_regs[100] = { 0 };
			VPUReq_t req = { 0 };

			memcpy(&m_regs, regs, nregs * sizeof(RK_U32));
			m_regs[64] = 0;
			m_regs[69] = 0;
			m_regs[70] = 0;
			m_regs[71] = 0;
			((Sw68_PerSel_t *)&m_regs[68])->perf_cnt0_sel = i;
			((Sw68_PerSel_t *)&m_regs[68])->perf_cnt1_sel = ((i + 1) > 40) ? 0 : (i + 1);
			((Sw68_PerSel_t *)&m_regs[68])->perf_cnt2_sel = ((i + 2) > 40) ? 0 : (i + 2);

			req.req = m_regs;
			req.size = nregs * sizeof(RK_U32);
			ret = (RK_S32)ioctl(socket, VPU_IOC_SET_REG, &req); 
			if (!ret){
				ret = (RK_S32)ioctl(socket, VPU_IOC_GET_REG, &req);
				if (!ret) {
					p->cycles[i + 0] = m_regs[69];
					p->cycles[i + 1] = m_regs[70];
					p->cycles[i + 2] = m_regs[71];

					p->total_cycles[i + 0] = m_regs[64];
					p->total_cycles[i + 1] = m_regs[64];
					p->total_cycles[i + 2] = m_regs[64];

					p->ddr_rdata[i + 0] = m_regs[65];
					p->ddr_rdata[i + 1] = m_regs[65];
					p->ddr_rdata[i + 2] = m_regs[65];
					p->ddr_wdata[i + 0] = m_regs[66];
					p->ddr_wdata[i + 1] = m_regs[66];
					p->ddr_wdata[i + 2] = m_regs[66];
				}
			}
		}
		if (p->fp_log) {
			RK_U64 aver_cycle = 0;
			RK_U64 aver_rdata = 0;
			RK_U64 aver_wdata = 0;
			for (i = 1; i <= 40; i++) {
				aver_cycle += p->total_cycles[i];
				aver_rdata += p->ddr_rdata[i];
				aver_wdata += p->ddr_wdata[i];
			}
			aver_cycle = (aver_cycle / 40);
			aver_rdata = (aver_rdata / 40);
			aver_wdata = (aver_wdata / 40);
			for (i = 1; i <= 40; i++) {
				if (1/*GetBitVal(p->choice_sel, i)*/) {
					float per_val = (float)(100 * p->cycles[i]) / (float)aver_cycle;
					per_val = (per_val > 100.0f) ? (100.0f) : per_val;
					fprintf(p->fp_log, "%3.2f, ", per_val);
				}
			}
			if (1/*GetBitVal(p->choice_sel, 41)*/) {
				fprintf(p->fp_log, "%lld, ", aver_cycle);
			}
			if (1/*GetBitVal(p->choice_sel, 42)*/) {
				fprintf(p->fp_log, "%lld, ", aver_rdata);
			}
			if (1/*GetBitVal(p->choice_sel, 43)*/) {
				fprintf(p->fp_log, "%lld, ", aver_wdata);
			}
			if (1/*GetBitVal(p->choice_sel, 44)*/) {
				fprintf(p->fp_log, "%d, \n", (regs[5]&0x7FFFFFF));
			}
		}
	}

	return ret;
}

int VPUClientInit(VPU_CLIENT_TYPE type)
{
    VPU_SERVICE_TEST;
    int ret;
    int fd;

	vpu_log_init(type, &g_vpu_debug_ctx);

    switch (type) {
        case VPU_DEC_RKV: {
            fd = open("/dev/rkvdec", O_RDWR);
            type = VPU_DEC;

            break;
        }
        case VPU_DEC_HEVC: {
            fd = open("/dev/hevc_service", O_RDWR);
            type = VPU_DEC;
            break;
        }
        case VPU_DEC_PP:
        case VPU_PP:
        case VPU_DEC:
        case VPU_ENC: {
            fd = open("/dev/vpu_service", O_RDWR);
            break;
        }
        default: {
            fd = -1;
            break;
        }
    }

    if (fd == -1) {
        mpp_err_f("failed to open /dev/rkvdec\n");
        return -1;
    }
    ret = ioctl(fd, VPU_IOC_SET_CLIENT_TYPE, (RK_U32)type);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_SET_CLIENT_TYPE failed ret %d errno %d\n", ret, errno);
        return -2;
    }
    return fd;
}

RK_S32 VPUClientRelease(int socket)
{
    VPU_SERVICE_TEST;
    int fd = socket;
    if (fd > 0) {
        close(fd);
    }
	vpu_log_deinit(&g_vpu_debug_ctx);
    return VPU_SUCCESS;
}

RK_S32 VPUClientSendReg(int socket, RK_U32 *regs, RK_U32 nregs)
{
    VPU_SERVICE_TEST;
    int fd = socket;
    RK_S32 ret;
    VPUReq_t req;
    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;
    ret = (RK_S32)ioctl(fd, VPU_IOC_SET_REG, &req);
    if (ret)
        mpp_err_f("ioctl VPU_IOC_SET_REG failed ret %d errno %d %s\n", ret, errno, strerror(errno));

    return ret;
}

RK_S32 VPUClientWaitResult(int socket, RK_U32 *regs, RK_U32 nregs, VPU_CMD_TYPE *cmd, RK_S32 *len)
{
    VPU_SERVICE_TEST;
    int fd = socket;
    RK_S32 ret;
    VPUReq_t req;
    (void)len;

	vpu_client_performance(socket, regs, nregs, &g_vpu_debug_ctx);

    nregs *= sizeof(RK_U32);
    req.req     = regs;
    req.size    = nregs;
    ret = (RK_S32)ioctl(fd, VPU_IOC_GET_REG, &req);
    if (ret) {
        mpp_err_f("ioctl VPU_IOC_GET_REG failed ret %d errno %d %s\n", ret, errno, strerror(errno));
        *cmd = VPU_SEND_CONFIG_ACK_FAIL;
    } else
        *cmd = VPU_SEND_CONFIG_ACK_OK;

    return ret;
}

RK_S32 VPUClientGetHwCfg(int socket, RK_U32 *cfg, RK_U32 cfg_size)
{
    VPU_SERVICE_TEST;
    int fd = socket;
    RK_S32 ret;
    VPUReq_t req;
    req.req     = cfg;
    req.size    = cfg_size;
    ret = (RK_S32)ioctl(fd, VPU_IOC_GET_HW_FUSE_STATUS, &req);
    if (ret)
        mpp_err_f("ioctl VPU_IOC_GET_HW_FUSE_STATUS failed ret %d\n", ret);

    return ret;
}

RK_U32 VPUCheckSupportWidth()
{
    VPUHwDecConfig_t hwCfg;
    int fd = -1;
    fd = open("/dev/vpu_service", O_RDWR);
    memset(&hwCfg, 0, sizeof(VPUHwDecConfig_t));
    if (fd >= 0) {
        if (VPUClientGetHwCfg(fd, (RK_U32*)&hwCfg, sizeof(hwCfg))) {
            mpp_err_f("Get HwCfg failed\n");
            close(fd);
            return -1;
        }
        close(fd);
        fd = -1;
    }
    return hwCfg.maxDecPicWidth;
}

RK_S32 VPUClientGetIOMMUStatus()
{
    static RK_U32 once = 1;
    if (once) {
        mpp_log("get iommu status always return true\n");
        once = 0;
    }
    return 1;
}

