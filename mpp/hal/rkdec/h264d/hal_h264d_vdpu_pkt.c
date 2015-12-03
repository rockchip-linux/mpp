
/*
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

#define MODULE_TAG "hal_h264d_vdpu_pkt"

#include <stdio.h>
#include <string.h>

#include "mpp_mem.h"
#include "hal_task.h"

#include "dxva_syntax.h"
#include "vpu.h"

#include "h264d_log.h"
#include "h264d_syntax.h"
#include "hal_h264d_fifo.h"
#include "hal_h264d_api.h"
#include "hal_h264d_global.h"
#include "hal_h264d_vdpu_pkt.h"
#include "hal_h264d_vdpu_reg.h"


const RK_U32 H264_VDPU_Cabac_table[VDPU_CABAC_TAB_SIZE / 4] = {
    0x14f10236, 0x034a14f1, 0x0236034a, 0xe47fe968, 0xfa35ff36, 0x07330000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0029003f, 0x003f003f, 0xf7530456, 0x0061f948, 0x0d29033e, 0x000b0137,
    0x0045ef7f, 0xf3660052, 0xf94aeb6b, 0xe57fe17f, 0xe87fee5f, 0xe57feb72,
    0xe27fef7b, 0xf473f07a, 0xf573f43f, 0xfe44f154, 0xf368fd46, 0xf85df65a,
    0xe27fff4a, 0xfa61f95b, 0xec7ffc38, 0xfb52f94c, 0xea7df95d, 0xf557fd4d,
    0xfb47fc3f, 0xfc44f454, 0xf93ef941, 0x083d0538, 0xfe420140, 0x003dfe4e,
    0x01320734, 0x0a23002c, 0x0b26012d, 0x002e052c, 0x1f110133, 0x07321c13,
    0x10210e3e, 0xf36cf164, 0xf365f35b, 0xf45ef658, 0xf054f656, 0xf953f357,
    0xed5e0146, 0x0048fb4a, 0x123bf866, 0xf164005f, 0xfc4b0248, 0xf54bfd47,
    0x0f2ef345, 0x003e0041, 0x1525f148, 0x09391036, 0x003e0c48, 0x18000f09,
    0x08190d12, 0x0f090d13, 0x0a250c12, 0x061d1421, 0x0f1e042d, 0x013a003e,
    0x073d0c26, 0x0b2d0f27, 0x0b2a0d2c, 0x102d0c29, 0x0a311e22, 0x122a0a37,
    0x1133112e, 0x00591aed, 0x16ef1aef, 0x1ee71cec, 0x21e925e5, 0x21e928e4,
    0x26ef21f5, 0x28f129fa, 0x26012911, 0x1efa1b03, 0x1a1625f0, 0x23fc26f8,
    0x26fd2503, 0x26052a00, 0x23102716, 0x0e301b25, 0x153c0c44, 0x0261fd47,
    0xfa2afb32, 0xfd36fe3e, 0x003a013f, 0xfe48ff4a, 0xf75bfb43, 0xfb1bfd27,
    0xfe2c002e, 0xf040f844, 0xf64efa4d, 0xf656f45c, 0xf137f63c, 0xfa3efc41,
    0xf449f84c, 0xf950f758, 0xef6ef561, 0xec54f54f, 0xfa49fc4a, 0xf356f360,
    0xf561ed75, 0xf84efb21, 0xfc30fe35, 0xfd3ef347, 0xf64ff456, 0xf35af261,
    0x0000fa5d, 0xfa54f84f, 0x0042ff47, 0x003efe3c, 0xfe3bfb4b, 0xfd3efc3a,
    0xf742ff4f, 0x00470344, 0x0a2cf93e, 0x0f240e28, 0x101b0c1d, 0x012c1424,
    0x1220052a, 0x01300a3e, 0x112e0940, 0xf468f561, 0xf060f958, 0xf855f955,
    0xf755f358, 0x0442fd4d, 0xfd4cfa4c, 0x0a3aff4c, 0xff53f963, 0xf25f025f,
    0x004cfb4a, 0x0046f54b, 0x01440041, 0xf249033e, 0x043eff44, 0xf34b0b37,
    0x05400c46, 0x0f060613, 0x07100c0e, 0x120d0d0b, 0x0d0f0f10, 0x0c170d17,
    0x0f140e1a, 0x0e2c1128, 0x112f1811, 0x15151916, 0x1f1b161d, 0x13230e32,
    0x0a39073f, 0xfe4dfc52, 0xfd5e0945, 0xf46d24dd, 0x24de20e6, 0x25e22ce0,
    0x22ee22f1, 0x28f121f9, 0x23fb2100, 0x2602210d, 0x17230d3a, 0x1dfd1a00,
    0x161e1ff9, 0x23f122fd, 0x220324ff, 0x2205200b, 0x2305220c, 0x270b1e1d,
    0x221a1d27, 0x13421f15, 0x1f1f1932, 0xef78ec70, 0xee72f555, 0xf15cf259,
    0xe647f151, 0xf2500044, 0xf246e838, 0xe944e832, 0xf54a17f3, 0x1af328f1,
    0x31f22c03, 0x2d062c22, 0x21361352, 0xfd4bff17, 0x0122012b, 0x0036fe37,
    0x003d0140, 0x0044f75c, 0xf26af361, 0xf15af45a, 0xee58f649, 0xf74ff256,
    0xf649f646, 0xf645fb42, 0xf740fb3a, 0x023b15f6, 0x18f51cf8, 0x1cff1d03,
    0x1d092314, 0x1d240e43, 0x14f10236, 0x034a14f1, 0x0236034a, 0xe47fe968,
    0xfa35ff36, 0x07331721, 0x17021500, 0x01090031, 0xdb760539, 0xf34ef541,
    0x013e0c31, 0xfc491132, 0x1240092b, 0x1d001a43, 0x105a0968, 0xd27fec68,
    0x0143f34e, 0xf541013e, 0xfa56ef5f, 0xfa3d092d, 0xfd45fa51, 0xf5600637,
    0x0743fb56, 0x0258003a, 0xfd4cf65e, 0x05360445, 0xfd510058, 0xf943fb4a,
    0xfc4afb50, 0xf948013a, 0x0029003f, 0x003f003f, 0xf7530456, 0x0061f948,
    0x0d29033e, 0x002dfc4e, 0xfd60e57e, 0xe462e765, 0xe943e452, 0xec5ef053,
    0xea6eeb5b, 0xee66f35d, 0xe37ff95c, 0xfb59f960, 0xf36cfd2e, 0xff41ff39,
    0xf75dfd4a, 0xf75cf857, 0xe97e0536, 0x063c063b, 0x0645ff30, 0x0044fc45,
    0xf858fe55, 0xfa4eff4b, 0xf94d0236, 0x0532fd44, 0x0132062a, 0xfc51013f,
    0xfc460043, 0x0239fe4c, 0x0b230440, 0x013d0b23, 0x12190c18, 0x0d1d0d24,
    0xf65df949, 0xfe490d2e, 0x0931f964, 0x09350235, 0x0535fe3d, 0x00380038,
    0xf33ffb3c, 0xff3e0439, 0xfa450439, 0x0e270433, 0x0d440340, 0x013d093f,
    0x07321027, 0x052c0434, 0x0b30fb3c, 0xff3b003b, 0x1621052c, 0x0e2bff4e,
    0x003c0945, 0x0b1c0228, 0x032c0031, 0x002e022c, 0x0233002f, 0x0427023e,
    0x062e0036, 0x0336023a, 0x043f0633, 0x06390735, 0x06340637, 0x0b2d0e24,
    0x0835ff52, 0x0737fd4e, 0x0f2e161f, 0xff541907, 0x1ef91c03, 0x1c042000,
    0x22ff1e06, 0x1e062009, 0x1f131a1b, 0x1a1e2514, 0x1c221146, 0x0143053b,
    0x0943101e, 0x12201223, 0x161d181f, 0x1726122b, 0x14290b3f, 0x093b0940,
    0xff5efe59, 0xf76cfa4c, 0xfe2c002d, 0x0034fd40, 0xfe3bfc46, 0xfc4bf852,
    0xef66f74d, 0x0318002a, 0x00300037, 0xfa3bf947, 0xf453f557, 0xe277013a,
    0xfd1dff24, 0x0126022b, 0xfa37003a, 0x0040fd4a, 0xf65a0046, 0xfc1d051f,
    0x072a013b, 0xfe3afd48, 0xfd51f561, 0x003a0805, 0x0a0e0e12, 0x0d1b0228,
    0x003afd46, 0xfa4ff855, 0x0000f36a, 0xf06af657, 0xeb72ee6e, 0xf262ea6e,
    0xeb6aee67, 0xeb6be96c, 0xe670f660, 0xf45ffb5b, 0xf75dea5e, 0xfb560943,
    0xfc50f655, 0xff46073c, 0x093a053d, 0x0c320f32, 0x12311136, 0x0a29072e,
    0xff330731, 0x08340929, 0x062f0237, 0x0d290a2c, 0x06320535, 0x0d31043f,
    0x0640fe45, 0xfe3b0646, 0x0a2c091f, 0x0c2b0335, 0x0e220a26, 0xfd340d28,
    0x1120072c, 0x07260d32, 0x0a391a2b, 0x0e0b0b0e, 0x090b120b, 0x150917fe,
    0x20f120f1, 0x22eb27e9, 0x2adf29e1, 0x2ee426f4, 0x151d2de8, 0x35d330e6,
    0x41d52bed, 0x27f61e09, 0x121a141b, 0x0039f252, 0xfb4bed61, 0xdd7d1b00,
    0x1c001ffc, 0x1b062208, 0x1e0a1816, 0x21131620, 0x1a1f1529, 0x1a2c172f,
    0x10410e47, 0x083c063f, 0x11411518, 0x17141a17, 0x1b201c17, 0x1c181728,
    0x18201c1d, 0x172a1339, 0x1635163d, 0x0b560c28, 0x0b330e3b, 0xfc4ff947,
    0xfb45f746, 0xf842f644, 0xed49f445, 0xf046f143, 0xec3eed46, 0xf042ea41,
    0xec3f09fe, 0x1af721f7, 0x27f929fe, 0x2d033109, 0x2d1b243b, 0xfa42f923,
    0xf92af82d, 0xfb30f438, 0xfa3cfb3e, 0xf842f84c, 0xfb55fa51, 0xf64df951,
    0xef50ee49, 0xfc4af653, 0xf747f743, 0xff3df842, 0xf242003b, 0x023b15f3,
    0x21f227f9, 0x2efe3302, 0x3c063d11, 0x37222a3e, 0x14f10236, 0x034a14f1,
    0x0236034a, 0xe47fe968, 0xfa35ff36, 0x07331619, 0x22001000, 0xfe090429,
    0xe3760241, 0xfa47f34f, 0x05340932, 0xfd460a36, 0x1a221316, 0x28003902,
    0x29241a45, 0xd37ff165, 0xfc4cfa47, 0xf34f0534, 0x0645f35a, 0x0034082b,
    0xfe45fb52, 0xf660023b, 0x024bfd57, 0xfd640138, 0xfd4afa55, 0x003bfd51,
    0xf956fb5f, 0xff42ff4d, 0x0146fe56, 0xfb48003d, 0x0029003f, 0x003f003f,
    0xf7530456, 0x0061f948, 0x0d29033e, 0x0d0f0733, 0x0250d97f, 0xee5bef60,
    0xe651dd62, 0xe866e961, 0xe577e863, 0xeb6eee66, 0xdc7f0050, 0xfb59f95e,
    0xfc5c0027, 0x0041f154, 0xdd7ffe49, 0xf468f75b, 0xe17f0337, 0x07380737,
    0x083dfd35, 0x0044f94a, 0xf758f367, 0xf35bf759, 0xf25cf84c, 0xf457e96e,
    0xe869f64e, 0xec70ef63, 0xb27fba7f, 0xce7fd27f, 0xfc42fb4e, 0xfc47f848,
    0x023bff37, 0xf946fa4b, 0xf859de77, 0xfd4b2014, 0x1e16d47f, 0x0036fb3d,
    0x003aff3c, 0xfd3df843, 0xe754f24a, 0xfb410534, 0x0239003d, 0xf745f546,
    0x1237fc47, 0x003a073d, 0x09291219, 0x0920052b, 0x092f002c, 0x0033022e,
    0x1326fc42, 0x0f260c2a, 0x09220059, 0x042d0a1c, 0x0a1f21f5, 0x34d5120f,
    0x1c0023ea, 0x26e72200, 0x27ee20f4, 0x66a20000, 0x38f121fc, 0x1d0a25fb,
    0x33e327f7, 0x34de45c6, 0x43c12cfb, 0x200737e3, 0x20010000, 0x1b2421e7,
    0x22e224e4, 0x26e426e5, 0x22ee23f0, 0x22f220f8, 0x25fa2300, 0x1e0a1c12,
    0x1a191d29, 0x004b0248, 0x084d0e23, 0x121f1123, 0x151e112d, 0x142a122d,
    0x1b1a1036, 0x07421038, 0x0b490a43, 0xf674e970, 0xf147f93d, 0x0035fb42,
    0xf54df750, 0xf754f657, 0xde7feb65, 0xfd27fb35, 0xf93df54b, 0xf14def5b,
    0xe76be76f, 0xe47af54c, 0xf62cf634, 0xf639f73a, 0xf048f945, 0xfc45fb4a,
    0xf7560242, 0xf7220120, 0x0b1f0534, 0xfe37fe43, 0x0049f859, 0x03340704,
    0x0a081108, 0x10130325, 0xff3dfb49, 0xff46fc4e, 0x0000eb7e, 0xe97cec6e,
    0xe67ee77c, 0xef69e579, 0xe575ef66, 0xe675e574, 0xdf7af65f, 0xf264f85f,
    0xef6fe472, 0xfa59fe50, 0xfc52f755, 0xf851ff48, 0x05400143, 0x09380045,
    0x01450745, 0xf945fa43, 0xf04dfe40, 0x023dfa43, 0xfd400239, 0xfd41fd42,
    0x003e0933, 0xff42fe47, 0xfe4bff46, 0xf7480e3c, 0x1025002f, 0x12230b25,
    0x0c290a29, 0x02300c29, 0x0d29003b, 0x03321328, 0x03421232, 0x13fa12fa,
    0x0e001af4, 0x1ff021e7, 0x21ea25e4, 0x27e22ae2, 0x2fd62ddc, 0x31de29ef,
    0x200945b9, 0x3fc142c0, 0x4db636d9, 0x34dd29f6, 0x240028ff, 0x1e0e1c1a,
    0x17250c37, 0x0b4125df, 0x27dc28db, 0x26e22edf, 0x2ae228e8, 0x31e326f4,
    0x28f626fd, 0x2efb1f14, 0x1d1e192c, 0x0c300b31, 0x1a2d1616, 0x17161b15,
    0x21141a1c, 0x1e181b22, 0x122a1927, 0x12320c46, 0x15360e47, 0x0b531920,
    0x15311536, 0xfb55fa51, 0xf64df951, 0xef50ee49, 0xfc4af653, 0xf747f743,
    0xff3df842, 0xf242003b, 0x023b11f6, 0x20f32af7, 0x31fb3500, 0x4003440a,
    0x421b2f39, 0xfb470018, 0xff24fe2a, 0xfe34f739, 0xfa3ffc41, 0xfc43f952,
    0xfd51fd4c, 0xf948fa4e, 0xf448f244, 0xfd46fa4c, 0xfb42fb3e, 0x0039fc3d,
    0xf73c0136, 0x023a11f6, 0x20f32af7, 0x31fb3500, 0x4003440a, 0x421b2f39,
    0x14f10236, 0x034a14f1, 0x0236034a, 0xe47fe968, 0xfa35ff36, 0x07331d10,
    0x19000e00, 0xf633fd3e, 0xe5631a10, 0xfc55e866, 0x05390639, 0xef490e39,
    0x1428140a, 0x1d003600, 0x252a0c61, 0xe07fea75, 0xfe4afc55, 0xe8660539,
    0xfa5df258, 0xfa2c0437, 0xf559f167, 0xeb741339, 0x143a0454, 0x0660013f,
    0xfb55f36a, 0x053f064b, 0xfd5aff65, 0x0337fc4f, 0xfe4bf461, 0xf932013c,
    0x0029003f, 0x003f003f, 0xf7530456, 0x0061f948, 0x0d29033e, 0x0722f758,
    0xec7fdc7f, 0xef5bf25f, 0xe754e756, 0xf459ef5b, 0xe17ff24c, 0xee67f35a,
    0xdb7f0b50, 0x054c0254, 0x054efa37, 0x043df253, 0xdb7ffb4f, 0xf568f55b,
    0xe27f0041, 0xfe4f0048, 0xfc5cfa38, 0x0344f847, 0xf362fc56, 0xf458fb52,
    0xfd48fc43, 0xf848f059, 0xf745ff3b, 0x05420439, 0xfc47fe47, 0x023aff4a,
    0xfc2cff45, 0x003ef933, 0xfc2ffa2a, 0xfd29fa35, 0x084cf74e, 0xf5530934,
    0x0043fb5a, 0x0143f148, 0xfb4bf850, 0xeb53eb40, 0xf31fe740, 0xe35e094b,
    0x113ff84a, 0xfb23fe1b, 0x0d5b0341, 0xf945084d, 0xf642033e, 0xfd44ec51,
    0x001e0107, 0xfd17eb4a, 0x1042e97c, 0x11252cee, 0x32deea7f, 0x0427002a,
    0x07220b1d, 0x081f0625, 0x072a0328, 0x08210d2b, 0x0d24042f, 0x0337023a,
    0x063c082c, 0x0b2c0e2a, 0x07300438, 0x04340d25, 0x0931133a, 0x0a300c2d,
    0x00451421, 0x083f23ee, 0x21e71cfd, 0x180a1b00, 0x22f234d4, 0x27e81311,
    0x1f19241d, 0x1821220f, 0x1e141649, 0x1422131f, 0x1b2c1310, 0x0f240f24,
    0x151c1915, 0x1e141f0c, 0x1b10182a, 0x005d0e38, 0x0f391a26, 0xe87fe873,
    0xea52f73e, 0x0035003b, 0xf255f359, 0xf35ef55c, 0xe37feb64, 0xf239f443,
    0xf547f64d, 0xeb55f058, 0xe968f162, 0xdb7ff652, 0xf830f83d, 0xf842f946,
    0xf24bf64f, 0xf753f45c, 0xee6cfc4f, 0xea45f04b, 0xfe3a013a, 0xf34ef753,
    0xfc51f363, 0xf351fa26, 0xf33efa3a, 0xfe3bf049, 0xf64cf356, 0xf753f657,
    0x0000ea7f, 0xe77fe778, 0xe57fed72, 0xe975e776, 0xe675e871, 0xe476e178,
    0xdb7cf65e, 0xf166f663, 0xf36ace7f, 0xfb5c1139, 0xfb56f35e, 0xf45bfe4d,
    0x0047ff49, 0x0440f951, 0x05400f39, 0x01430044, 0xf6430144, 0x004d0240,
    0x0044fb4e, 0x0737053b, 0x02410e36, 0x0f2c053c, 0x0246fe4c, 0xee560c46,
    0x0540f446, 0x0b370538, 0x00450241, 0xfa4a0536, 0x0736fa4c, 0xf552fe4d,
    0xfe4d192a, 0x11f310f7, 0x11f41beb, 0x25e229d8, 0x2ad730d1, 0x27e02ed8,
    0x34cd2ed7, 0x34d92bed, 0x200b3dc9, 0x38d23ece, 0x51bd2dec, 0x23fe1c0f,
    0x22012701, 0x1e111426, 0x122d0f36, 0x004f24f0, 0x25f225ef, 0x2001220f,
    0x1d0f1819, 0x22161f10, 0x23121f1c, 0x2129241c, 0x1b2f153e, 0x121f131a,
    0x24181817, 0x1b10181e, 0x1f1d1629, 0x162a103c, 0x0f340e3c, 0x034ef07b,
    0x15351638, 0x193d1521, 0x1332113d, 0xfd4ef84a, 0xf748f648, 0xee4bf447,
    0xf53ffb46, 0xef4bf248, 0xf043f835, 0xf23bf734, 0xf54409fe, 0x1ef61ffc,
    0x21ff2107, 0x1f0c2517, 0x1f261440, 0xf747f925, 0xf82cf531, 0xf638f43b,
    0xf83ff743, 0xfa44f64f, 0xfd4ef84a, 0xf748f648, 0xee4bf447, 0xf53ffb46,
    0xef4bf248, 0xf043f835, 0xf23bf734, 0xf54409fe, 0x1ef61ffc, 0x21ff2107,
    0x1f0c2517, 0x1f261440,
};


const HalRegDrv_t g_vdpu_drv[VDPU_MAX_SIZE + 1] = {
    { VDPU_DEC_PIC_INF       , 1 ,  1, 24, "sw01_dec_pic_inf      " },
    { VDPU_DEC_TIMEOUT       , 1 ,  1, 18, "sw01_dec_timeout      " },
    { VDPU_DEC_SLICE_INT     , 1 ,  1, 17, "sw01_dec_slice_int    " },
    { VDPU_DEC_ERROR_INT     , 1 ,  1, 16, "sw01_dec_error_int    " },
    { VDPU_DEC_ASO_INT       , 1 ,  1, 15, "sw01_dec_aso_int      " },
    { VDPU_DEC_BUFFER_INT    , 1 ,  1, 14, "sw01_dec_buffer_int   " },
    { VDPU_DEC_BUS_INT       , 1 ,  1, 13, "sw01_dec_bus_int      " },
    { VDPU_DEC_RDY_INT       , 1 ,  1, 12, "sw01_dec_rdy_int      " },
    { VDPU_DEC_IRQ           , 1 ,  1,  8, "sw01_dec_irq          " },
    { VDPU_DEC_IRQ_DIS       , 1 ,  1,  4, "sw01_dec_irq_dis      " },
    { VDPU_DEC_E             , 1 ,  1,  0, "sw01_dec_e            " },
    { VDPU_DEC_AXI_RD_ID     , 2 ,  8, 24, "sw02_dec_axi_rd_id    " },
    { VDPU_DEC_TIMEOUT_E     , 2 ,  1, 23, "sw02_dec_timeout_e    " },
    { VDPU_DEC_STRSWAP32_E   , 2 ,  1, 22, "sw02_dec_strswap32_e  " },
    { VDPU_DEC_STRENDIAN_E   , 2 ,  1, 21, "sw02_dec_strendian_e  " },
    { VDPU_DEC_INSWAP32_E    , 2 ,  1, 20, "sw02_dec_inswap32_e   " },
    { VDPU_DEC_OUTSWAP32_E   , 2 ,  1, 19, "sw02_dec_outswap32_e  " },
    { VDPU_DEC_DATA_DISC_E   , 2 ,  1, 18, "sw02_dec_data_disc_e  " },
    { VDPU_DEC_OUT_TILED_E   , 2 ,  1, 17, "sw02_dec_out_tiled_e  " },
    { VDPU_DEC_LATENCY       , 2 ,  6, 11, "sw02_dec_latency      " },
    { VDPU_DEC_CLK_GATE_E    , 2 ,  1, 10, "sw02_dec_clk_gate_e   " },
    { VDPU_DEC_IN_ENDIAN     , 2 ,  1,  9, "sw02_dec_in_endian    " },
    { VDPU_DEC_OUT_ENDIAN    , 2 ,  1,  8, "sw02_dec_out_endian   " },
    { VDPU_PRIORITY_MODE     , 2 ,  3,  5, "sw02_priority_mode    " },
    { VDPU_DEC_ADV_PRE_DIS   , 2 ,  1,  6, "sw02_dec_adv_pre_dis  " },
    { VDPU_DEC_SCMD_DIS      , 2 ,  1,  5, "sw02_dec_scmd_dis     " },
    { VDPU_DEC_MAX_BURST     , 2 ,  5,  0, "sw02_dec_max_burst    " },
    { VDPU_DEC_MODE          , 3 ,  4, 28, "sw03_dec_mode         " },
    { VDPU_RLC_MODE_E        , 3 ,  1, 27, "sw03_rlc_mode_e       " },
    { VDPU_SKIP_MODE         , 3 ,  1, 26, "sw03_skip_mode        " },
    { VDPU_DIVX3_E           , 3 ,  1, 25, "sw03_divx3_e          " },
    { VDPU_PJPEG_E           , 3 ,  1, 24, "sw03_pjpeg_e          " },
    { VDPU_PIC_INTERLACE_E   , 3 ,  1, 23, "sw03_pic_interlace_e  " },
    { VDPU_PIC_FIELDMODE_E   , 3 ,  1, 22, "sw03_pic_fieldmode_e  " },
    { VDPU_PIC_B_E           , 3 ,  1, 21, "sw03_pic_b_e          " },
    { VDPU_PIC_INTER_E       , 3 ,  1, 20, "sw03_pic_inter_e      " },
    { VDPU_PIC_TOPFIELD_E    , 3 ,  1, 19, "sw03_pic_topfield_e   " },
    { VDPU_FWD_INTERLACE_E   , 3 ,  1, 18, "sw03_fwd_interlace_e  " },
    { VDPU_SORENSON_E        , 3 ,  1, 17, "sw03_sorenson_e       " },
    { VDPU_REF_TOPFIELD_E    , 3 ,  1, 16, "sw03_ref_topfield_e   " },
    { VDPU_DEC_OUT_DIS       , 3 ,  1, 15, "sw03_dec_out_dis      " },
    { VDPU_FILTERING_DIS     , 3 ,  1, 14, "sw03_filtering_dis    " },
    { VDPU_PIC_FIXED_QUANT   , 3 ,  1, 13, "sw03_pic_fixed_quant  " },
	{ VDPU_MVC_E             , 3 ,  1, 13, "sw03_mvc_e            " },
    { VDPU_WRITE_MVS_E       , 3 ,  1, 12, "sw03_write_mvs_e      " },
    { VDPU_REFTOPFIRST_E     , 3 ,  1, 11, "sw03_reftopfirst_e    " },
    { VDPU_SEQ_MBAFF_E       , 3 ,  1, 10, "sw03_seq_mbaff_e      " },
    { VDPU_PICORD_COUNT_E    , 3 ,  1,  9, "sw03_picord_count_e   " },
    { VDPU_DEC_AHB_HLOCK_E   , 3 ,  1,  8, "sw03_dec_ahb_hlock_e  " },
    { VDPU_DEC_AXI_WR_ID     , 3 ,  8,  0, "sw03_dec_axi_wr_id    " },
    { VDPU_PIC_MB_WIDTH      , 4 ,  9, 23, "sw04_pic_mb_width     " },
    { VDPU_MB_WIDTH_OFF      , 4 ,  4, 19, "sw04_mb_width_off     " },
    { VDPU_PIC_MB_HEIGHT_P   , 4 ,  8, 11, "sw04_pic_mb_height_p  " },
    { VDPU_MB_HEIGHT_OFF     , 4 ,  4,  7, "sw04_mb_height_off    " },
    { VDPU_ALT_SCAN_E        , 4 ,  1,  6, "sw04_alt_scan_e       " },
    { VDPU_TOPFIELDFIRST_E   , 4 ,  1,  5, "sw04_topfieldfirst_e  " },
    { VDPU_REF_FRAMES        , 4 ,  5,  0, "sw04_ref_frames       " },
    { VDPU_PIC_MB_W_EXT      , 4 ,  3,  3, "sw04_pic_mb_w_ext     " },
    { VDPU_PIC_MB_H_EXT      , 4 ,  3,  0, "sw04_pic_mb_h_ext     " },
    { VDPU_PIC_REFER_FLAG    , 4 ,  1,  0, "sw04_pic_refer_flag   " },
    { VDPU_STRM_START_BIT    , 5 ,  6, 26, "sw05_strm_start_bit   " },
    { VDPU_SYNC_MARKER_E     , 5 ,  1, 25, "sw05_sync_marker_e    " },
    { VDPU_TYPE1_QUANT_E     , 5 ,  1, 24, "sw05_type1_quant_e    " },
    { VDPU_CH_QP_OFFSET      , 5 ,  5, 19, "sw05_ch_qp_offset     " },
    { VDPU_CH_QP_OFFSET2     , 5 ,  5, 14, "sw05_ch_qp_offset2    " },
    { VDPU_FIELDPIC_FLAG_E   , 5 ,  1,  0, "sw05_fieldpic_flag_e  " },
    { VDPU_INTRADC_VLC_THR   , 5 ,  3, 16, "sw05_intradc_vlc_thr  " },
    { VDPU_VOP_TIME_INCR     , 5 , 16,  0, "sw05_vop_time_incr    " },
    { VDPU_DQ_PROFILE        , 5 ,  1, 24, "sw05_dq_profile       " },
    { VDPU_DQBI_LEVEL        , 5 ,  1, 23, "sw05_dqbi_level       " },
    { VDPU_RANGE_RED_FRM_E   , 5 ,  1, 22, "sw05_range_red_frm_e  " },
    { VDPU_FAST_UVMC_E       , 5 ,  1, 20, "sw05_fast_uvmc_e      " },
    { VDPU_TRANSDCTAB        , 5 ,  1, 17, "sw05_transdctab       " },
    { VDPU_TRANSACFRM        , 5 ,  2, 15, "sw05_transacfrm       " },
    { VDPU_TRANSACFRM2       , 5 ,  2, 13, "sw05_transacfrm2      " },
    { VDPU_MB_MODE_TAB       , 5 ,  3, 10, "sw05_mb_mode_tab      " },
    { VDPU_MVTAB             , 5 ,  3,  7, "sw05_mvtab            " },
    { VDPU_CBPTAB            , 5 ,  3,  4, "sw05_cbptab           " },
    { VDPU_2MV_BLK_PAT_TAB   , 5 ,  2,  2, "sw05_2mv_blk_pat_tab  " },
    { VDPU_4MV_BLK_PAT_TAB   , 5 ,  2,  0, "sw05_4mv_blk_pat_tab  " },
    { VDPU_QSCALE_TYPE       , 5 ,  1, 24, "sw05_qscale_type      " },
    { VDPU_CON_MV_E          , 5 ,  1,  4, "sw05_con_mv_e         " },
    { VDPU_INTRA_DC_PREC     , 5 ,  2,  2, "sw05_intra_dc_prec    " },
    { VDPU_INTRA_VLC_TAB     , 5 ,  1,  1, "sw05_intra_vlc_tab    " },
    { VDPU_FRAME_PRED_DCT    , 5 ,  1,  0, "sw05_frame_pred_dct   " },
    { VDPU_JPEG_QTABLES      , 5 ,  2, 11, "sw05_jpeg_qtables     " },
    { VDPU_JPEG_MODE         , 5 ,  3,  8, "sw05_jpeg_mode        " },
    { VDPU_JPEG_FILRIGHT_E   , 5 ,  1,  7, "sw05_jpeg_filright_e  " },
    { VDPU_JPEG_STREAM_ALL   , 5 ,  1,  6, "sw05_jpeg_stream_all  " },
    { VDPU_CR_AC_VLCTABLE    , 5 ,  1,  5, "sw05_cr_ac_vlctable   " },
    { VDPU_CB_AC_VLCTABLE    , 5 ,  1,  4, "sw05_cb_ac_vlctable   " },
    { VDPU_CR_DC_VLCTABLE    , 5 ,  1,  3, "sw05_cr_dc_vlctable   " },
    { VDPU_CB_DC_VLCTABLE    , 5 ,  1,  2, "sw05_cb_dc_vlctable   " },
    { VDPU_CR_DC_VLCTABLE3   , 5 ,  1,  1, "sw05_cr_dc_vlctable3  " },
    { VDPU_CB_DC_VLCTABLE3   , 5 ,  1,  0, "sw05_cb_dc_vlctable3  " },
    { VDPU_STRM1_START_BIT   , 5 ,  6, 18, "sw05_strm1_start_bit  " },
    { VDPU_HUFFMAN_E         , 5 ,  1, 17, "sw05_huffman_e        " },
    { VDPU_MULTISTREAM_E     , 5 ,  1, 16, "sw05_multistream_e    " },
    { VDPU_BOOLEAN_VALUE     , 5 ,  8,  8, "sw05_boolean_value    " },
    { VDPU_BOOLEAN_RANGE     , 5 ,  8,  0, "sw05_boolean_range    " },
    { VDPU_ALPHA_OFFSET      , 5 ,  5,  5, "sw05_alpha_offset     " },
    { VDPU_BETA_OFFSET       , 5 ,  5,  0, "sw05_beta_offset      " },
    { VDPU_START_CODE_E      , 6 ,  1, 31, "sw06_start_code_e     " },
    { VDPU_INIT_QP           , 6 ,  6, 25, "sw06_init_qp          " },
    { VDPU_CH_8PIX_ILEAV_E   , 6 ,  1, 24, "sw06_ch_8pix_ileav_e  " },
    { VDPU_STREAM_LEN        , 6 , 24,  0, "sw06_stream_len       " },
    { VDPU_CABAC_E           , 7 ,  1, 31, "sw07_cabac_e          " },
    { VDPU_BLACKWHITE_E      , 7 ,  1, 30, "sw07_blackwhite_e     " },
    { VDPU_DIR_8X8_INFER_E   , 7 ,  1, 29, "sw07_dir_8x8_infer_e  " },
    { VDPU_WEIGHT_PRED_E     , 7 ,  1, 28, "sw07_weight_pred_e    " },
    { VDPU_WEIGHT_BIPR_IDC   , 7 ,  2, 26, "sw07_weight_bipr_idc  " },
    { VDPU_FRAMENUM_LEN      , 7 ,  5, 16, "sw07_framenum_len     " },
    { VDPU_FRAMENUM          , 7 , 16,  0, "sw07_framenum         " },
    { VDPU_BITPLANE0_E       , 7 ,  1, 31, "sw07_bitplane0_e      " },
    { VDPU_BITPLANE1_E       , 7 ,  1, 30, "sw07_bitplane1_e      " },
    { VDPU_BITPLANE2_E       , 7 ,  1, 29, "sw07_bitplane2_e      " },
    { VDPU_ALT_PQUANT        , 7 ,  5, 24, "sw07_alt_pquant       " },
    { VDPU_DQ_EDGES          , 7 ,  4, 20, "sw07_dq_edges         " },
    { VDPU_TTMBF             , 7 ,  1, 19, "sw07_ttmbf            " },
    { VDPU_PQINDEX           , 7 ,  5, 14, "sw07_pqindex          " },
    { VDPU_BILIN_MC_E        , 7 ,  1, 12, "sw07_bilin_mc_e       " },
    { VDPU_UNIQP_E           , 7 ,  1, 11, "sw07_uniqp_e          " },
    { VDPU_HALFQP_E          , 7 ,  1, 10, "sw07_halfqp_e         " },
    { VDPU_TTFRM             , 7 ,  2,  8, "sw07_ttfrm            " },
    { VDPU_2ND_BYTE_EMUL_E   , 7 ,  1,  7, "sw07_2nd_byte_emul_e  " },
    { VDPU_DQUANT_E          , 7 ,  1,  6, "sw07_dquant_e         " },
    { VDPU_VC1_ADV_E         , 7 ,  1,  5, "sw07_vc1_adv_e        " },
    { VDPU_PJPEG_FILDOWN_E   , 7 ,  1, 26, "sw07_pjpeg_fildown_e  " },
    { VDPU_PJPEG_WDIV8       , 7 ,  1, 25, "sw07_pjpeg_wdiv8      " },
    { VDPU_PJPEG_HDIV8       , 7 ,  1, 24, "sw07_pjpeg_hdiv8      " },
    { VDPU_PJPEG_AH          , 7 ,  4, 20, "sw07_pjpeg_ah         " },
    { VDPU_PJPEG_AL          , 7 ,  4, 16, "sw07_pjpeg_al         " },
    { VDPU_PJPEG_SS          , 7 ,  8,  8, "sw07_pjpeg_ss         " },
    { VDPU_PJPEG_SE          , 7 ,  8,  0, "sw07_pjpeg_se         " },
    { VDPU_DCT1_START_BIT    , 7 ,  6, 26, "sw07_dct1_start_bit   " },
    { VDPU_DCT2_START_BIT    , 7 ,  6, 20, "sw07_dct2_start_bit   " },
    { VDPU_CH_MV_RES         , 7 ,  1, 13, "sw07_ch_mv_res        " },
    { VDPU_INIT_DC_MATCH0    , 7 ,  3,  9, "sw07_init_dc_match0   " },
    { VDPU_INIT_DC_MATCH1    , 7 ,  3,  6, "sw07_init_dc_match1   " },
    { VDPU_VP7_VERSION       , 7 ,  1,  5, "sw07_vp7_version      " },
    { VDPU_CONST_INTRA_E     , 8 ,  1, 31, "sw08_const_intra_e    " },
    { VDPU_FILT_CTRL_PRES    , 8 ,  1, 30, "sw08_filt_ctrl_pres   " },
    { VDPU_RDPIC_CNT_PRES    , 8 ,  1, 29, "sw08_rdpic_cnt_pres   " },
    { VDPU_8X8TRANS_FLAG_E   , 8 ,  1, 28, "sw08_8x8trans_flag_e  " },
    { VDPU_REFPIC_MK_LEN     , 8 , 11, 17, "sw08_refpic_mk_len    " },
    { VDPU_IDR_PIC_E         , 8 ,  1, 16, "sw08_idr_pic_e        " },
    { VDPU_IDR_PIC_ID        , 8 , 16,  0, "sw08_idr_pic_id       " },
    { VDPU_MV_SCALEFACTOR    , 8 ,  8, 24, "sw08_mv_scalefactor   " },
    { VDPU_REF_DIST_FWD      , 8 ,  5, 19, "sw08_ref_dist_fwd     " },
    { VDPU_REF_DIST_BWD      , 8 ,  5, 14, "sw08_ref_dist_bwd     " },
    { VDPU_LOOP_FILT_LIMIT   , 8 ,  4, 14, "sw08_loop_filt_limit  " },
    { VDPU_VARIANCE_TEST_E   , 8 ,  1, 13, "sw08_variance_test_e  " },
    { VDPU_MV_THRESHOLD      , 8 ,  3, 10, "sw08_mv_threshold     " },
    { VDPU_VAR_THRESHOLD     , 8 , 10,  0, "sw08_var_threshold    " },
    { VDPU_DIVX_IDCT_E       , 8 ,  1,  8, "sw08_divx_idct_e      " },
    { VDPU_DIVX3_SLICE_SIZE  , 8 ,  8,  0, "sw08_divx3_slice_size " },
    { VDPU_PJPEG_REST_FREQ   , 8 , 16,  0, "sw08_pjpeg_rest_freq  " },
    { VDPU_RV_PROFILE        , 8 ,  2, 30, "sw08_rv_profile       " },
    { VDPU_RV_OSV_QUANT      , 8 ,  2, 28, "sw08_rv_osv_quant     " },
    { VDPU_RV_FWD_SCALE      , 8 , 14, 14, "sw08_rv_fwd_scale     " },
    { VDPU_RV_BWD_SCALE      , 8 , 14,  0, "sw08_rv_bwd_scale     " },
    { VDPU_INIT_DC_COMP0     , 8 , 16, 16, "sw08_init_dc_comp0    " },
    { VDPU_INIT_DC_COMP1     , 8 , 16,  0, "sw08_init_dc_comp1    " },
    { VDPU_PPS_ID            , 9 ,  8, 24, "sw09_pps_id           " },
    { VDPU_REFIDX1_ACTIVE    , 9 ,  5, 19, "sw09_refidx1_active   " },
    { VDPU_REFIDX0_ACTIVE    , 9 ,  5, 14, "sw09_refidx0_active   " },
    { VDPU_POC_LENGTH        , 9 ,  8,  0, "sw09_poc_length       " },
    { VDPU_ICOMP0_E          , 9 ,  1, 24, "sw09_icomp0_e         " },
    { VDPU_ISCALE0           , 9 ,  8, 16, "sw09_iscale0          " },
    { VDPU_ISHIFT0           , 9 , 16,  0, "sw09_ishift0          " },
    { VDPU_STREAM1_LEN       , 9 , 24,  0, "sw09_stream1_len      " },
    { VDPU_MB_CTRL_BASE      , 9 , 32,  0, "sw09_mb_ctrl_base     " },
    { VDPU_PIC_SLICE_AM      , 9 , 13,  0, "sw09_pic_slice_am     " },
    { VDPU_COEFFS_PART_AM    , 9 ,  4, 24, "sw09_coeffs_part_am   " },
    { VDPU_DIFF_MV_BASE      , 10, 32,  0, "sw10_diff_mv_base     " },
    { VDPU_PINIT_RLIST_F9    , 10,  5, 25, "sw10_pinit_rlist_f9   " },
    { VDPU_PINIT_RLIST_F8    , 10,  5, 20, "sw10_pinit_rlist_f8   " },
    { VDPU_PINIT_RLIST_F7    , 10,  5, 15, "sw10_pinit_rlist_f7   " },
    { VDPU_PINIT_RLIST_F6    , 10,  5, 10, "sw10_pinit_rlist_f6   " },
    { VDPU_PINIT_RLIST_F5    , 10,  5,  5, "sw10_pinit_rlist_f5   " },
    { VDPU_PINIT_RLIST_F4    , 10,  5,  0, "sw10_pinit_rlist_f4   " },
    { VDPU_ICOMP1_E          , 10,  1, 24, "sw10_icomp1_e         " },
    { VDPU_ISCALE1           , 10,  8, 16, "sw10_iscale1          " },
    { VDPU_ISHIFT1           , 10, 16,  0, "sw10_ishift1          " },
    { VDPU_SEGMENT_BASE      , 10, 32,  0, "sw10_segment_base     " },
    { VDPU_SEGMENT_UPD_E     , 10,  1,  1, "sw10_segment_upd_e    " },
    { VDPU_SEGMENT_E         , 10,  1,  0, "sw10_segment_e        " },
    { VDPU_I4X4_OR_DC_BASE   , 11, 32,  0, "sw11_i4x4_or_dc_base  " },
    { VDPU_PINIT_RLIST_F15   , 11,  5, 25, "sw11_pinit_rlist_f15  " },
    { VDPU_PINIT_RLIST_F14   , 11,  5, 20, "sw11_pinit_rlist_f14  " },
    { VDPU_PINIT_RLIST_F13   , 11,  5, 15, "sw11_pinit_rlist_f13  " },
    { VDPU_PINIT_RLIST_F12   , 11,  5, 10, "sw11_pinit_rlist_f12  " },
    { VDPU_PINIT_RLIST_F11   , 11,  5,  5, "sw11_pinit_rlist_f11  " },
    { VDPU_PINIT_RLIST_F10   , 11,  5,  0, "sw11_pinit_rlist_f10  " },
    { VDPU_ICOMP2_E          , 11,  1, 24, "sw11_icomp2_e         " },
    { VDPU_ISCALE2           , 11,  8, 16, "sw11_iscale2          " },
    { VDPU_ISHIFT2           , 11, 16,  0, "sw11_ishift2          " },
    { VDPU_DCT3_START_BIT    , 11,  6, 24, "sw11_dct3_start_bit   " },
    { VDPU_DCT4_START_BIT    , 11,  6, 18, "sw11_dct4_start_bit   " },
    { VDPU_DCT5_START_BIT    , 11,  6, 12, "sw11_dct5_start_bit   " },
    { VDPU_DCT6_START_BIT    , 11,  6,  6, "sw11_dct6_start_bit   " },
    { VDPU_DCT7_START_BIT    , 11,  6,  0, "sw11_dct7_start_bit   " },
    { VDPU_RLC_VLC_BASE      , 12, 32,  0, "sw12_rlc_vlc_base     " },
    { VDPU_DEC_OUT_BASE      , 13, 32,  0, "sw13_dec_out_base     " },
    { VDPU_REFER0_BASE       , 14, 32,  0, "sw14_refer0_base      " },
    { VDPU_REFER0_FIELD_E    , 14,  1,  1, "sw14_refer0_field_e   " },
    { VDPU_REFER0_TOPC_E     , 14,  1,  0, "sw14_refer0_topc_e    " },
    { VDPU_JPG_CH_OUT_BASE   , 14, 32,  0, "sw14_jpg_ch_out_base  " },
    { VDPU_REFER1_BASE       , 15, 32,  0, "sw15_refer1_base      " },
    { VDPU_REFER1_FIELD_E    , 15,  1,  1, "sw15_refer1_field_e   " },
    { VDPU_REFER1_TOPC_E     , 15,  1,  0, "sw15_refer1_topc_e    " },
    { VDPU_JPEG_SLICE_H      , 15,  8,  0, "sw15_jpeg_slice_h     " },
    { VDPU_REFER2_BASE       , 16, 32,  0, "sw16_refer2_base      " },
    { VDPU_REFER2_FIELD_E    , 16,  1,  1, "sw16_refer2_field_e   " },
    { VDPU_REFER2_TOPC_E     , 16,  1,  0, "sw16_refer2_topc_e    " },
    { VDPU_AC1_CODE6_CNT     , 16,  7, 24, "sw16_ac1_code6_cnt    " },
    { VDPU_AC1_CODE5_CNT     , 16,  6, 16, "sw16_ac1_code5_cnt    " },
    { VDPU_AC1_CODE4_CNT     , 16,  5, 11, "sw16_ac1_code4_cnt    " },
    { VDPU_AC1_CODE3_CNT     , 16,  4,  7, "sw16_ac1_code3_cnt    " },
    { VDPU_AC1_CODE2_CNT     , 16,  3,  3, "sw16_ac1_code2_cnt    " },
    { VDPU_AC1_CODE1_CNT     , 16,  2,  0, "sw16_ac1_code1_cnt    " },
    { VDPU_REFER3_BASE       , 17, 32,  0, "sw17_refer3_base      " },
    { VDPU_REFER3_FIELD_E    , 17,  1,  1, "sw17_refer3_field_e   " },
    { VDPU_REFER3_TOPC_E     , 17,  1,  0, "sw17_refer3_topc_e    " },
    { VDPU_AC1_CODE10_CNT    , 17,  8, 24, "sw17_ac1_code10_cnt   " },
    { VDPU_AC1_CODE9_CNT     , 17,  8, 16, "sw17_ac1_code9_cnt    " },
    { VDPU_AC1_CODE8_CNT     , 17,  8,  8, "sw17_ac1_code8_cnt    " },
    { VDPU_AC1_CODE7_CNT     , 17,  8,  0, "sw17_ac1_code7_cnt    " },
    { VDPU_REFER4_BASE       , 18, 32,  0, "sw18_refer4_base      " },
    { VDPU_REFER4_FIELD_E    , 18,  1,  1, "sw18_refer4_field_e   " },
    { VDPU_REFER4_TOPC_E     , 18,  1,  0, "sw18_refer4_topc_e    " },
    { VDPU_PIC_HEADER_LEN    , 18, 16, 16, "sw18_pic_header_len   " },
    { VDPU_PIC_4MV_E         , 18,  1, 13, "sw18_pic_4mv_e        " },
    { VDPU_RANGE_RED_REF_E   , 18,  1, 11, "sw18_range_red_ref_e  " },
    { VDPU_VC1_DIFMV_RANGE   , 18,  2,  9, "sw18_vc1_difmv_range  " },
    { VDPU_MV_RANGE          , 18,  2,  6, "sw18_mv_range         " },
    { VDPU_OVERLAP_E         , 18,  1,  5, "sw18_overlap_e        " },
    { VDPU_OVERLAP_METHOD    , 18,  2,  3, "sw18_overlap_method   " },
    { VDPU_ALT_SCAN_FLAG_E   , 18,  1, 19, "sw18_alt_scan_flag_e  " },
    { VDPU_FCODE_FWD_HOR     , 18,  4, 15, "sw18_fcode_fwd_hor    " },
    { VDPU_FCODE_FWD_VER     , 18,  4, 11, "sw18_fcode_fwd_ver    " },
    { VDPU_FCODE_BWD_HOR     , 18,  4,  7, "sw18_fcode_bwd_hor    " },
    { VDPU_FCODE_BWD_VER     , 18,  4,  3, "sw18_fcode_bwd_ver    " },
    { VDPU_MV_ACCURACY_FWD   , 18,  1,  2, "sw18_mv_accuracy_fwd  " },
    { VDPU_MV_ACCURACY_BWD   , 18,  1,  1, "sw18_mv_accuracy_bwd  " },
    { VDPU_MPEG4_VC1_RC      , 18,  1,  1, "sw18_mpeg4_vc1_rc     " },
    { VDPU_PREV_ANC_TYPE     , 18,  1,  0, "sw18_prev_anc_type    " },
    { VDPU_AC1_CODE14_CNT    , 18,  8, 24, "sw18_ac1_code14_cnt   " },
    { VDPU_AC1_CODE13_CNT    , 18,  8, 16, "sw18_ac1_code13_cnt   " },
    { VDPU_AC1_CODE12_CNT    , 18,  8,  8, "sw18_ac1_code12_cnt   " },
    { VDPU_AC1_CODE11_CNT    , 18,  8,  0, "sw18_ac1_code11_cnt   " },
    { VDPU_GREF_SIGN_BIAS    , 18,  1,  0, "sw18_gref_sign_bias   " },
    { VDPU_REFER5_BASE       , 19, 32,  0, "sw19_refer5_base      " },
    { VDPU_REFER5_FIELD_E    , 19,  1,  1, "sw19_refer5_field_e   " },
    { VDPU_REFER5_TOPC_E     , 19,  1,  0, "sw19_refer5_topc_e    " },
    { VDPU_TRB_PER_TRD_D0    , 19, 27,  0, "sw19_trb_per_trd_d0   " },
    { VDPU_ICOMP3_E          , 19,  1, 24, "sw19_icomp3_e         " },
    { VDPU_ISCALE3           , 19,  8, 16, "sw19_iscale3          " },
    { VDPU_ISHIFT3           , 19, 16,  0, "sw19_ishift3          " },
    { VDPU_AC2_CODE4_CNT     , 19,  5, 27, "sw19_ac2_code4_cnt    " },
    { VDPU_AC2_CODE3_CNT     , 19,  4, 23, "sw19_ac2_code3_cnt    " },
    { VDPU_AC2_CODE2_CNT     , 19,  3, 19, "sw19_ac2_code2_cnt    " },
    { VDPU_AC2_CODE1_CNT     , 19,  2, 16, "sw19_ac2_code1_cnt    " },
    { VDPU_AC1_CODE16_CNT    , 19,  8,  8, "sw19_ac1_code16_cnt   " },
    { VDPU_AC1_CODE15_CNT    , 19,  8,  0, "sw19_ac1_code15_cnt   " },
    { VDPU_SCAN_MAP_1        , 19,  6, 24, "sw19_scan_map_1       " },
    { VDPU_SCAN_MAP_2        , 19,  6, 18, "sw19_scan_map_2       " },
    { VDPU_SCAN_MAP_3        , 19,  6, 12, "sw19_scan_map_3       " },
    { VDPU_SCAN_MAP_4        , 19,  6,  6, "sw19_scan_map_4       " },
    { VDPU_SCAN_MAP_5        , 19,  6,  0, "sw19_scan_map_5       " },
    { VDPU_AREF_SIGN_BIAS    , 19,  1,  0, "sw19_aref_sign_bias   " },
    { VDPU_REFER6_BASE       , 20, 32,  0, "sw20_refer6_base      " },
    { VDPU_REFER6_FIELD_E    , 20,  1,  1, "sw20_refer6_field_e   " },
    { VDPU_REFER6_TOPC_E     , 20,  1,  0, "sw20_refer6_topc_e    " },
    { VDPU_TRB_PER_TRD_DM1   , 20, 27,  0, "sw20_trb_per_trd_dm1  " },
    { VDPU_ICOMP4_E          , 20,  1, 24, "sw20_icomp4_e         " },
    { VDPU_ISCALE4           , 20,  8, 16, "sw20_iscale4          " },
    { VDPU_ISHIFT4           , 20, 16,  0, "sw20_ishift4          " },
    { VDPU_AC2_CODE8_CNT     , 20,  8, 24, "sw20_ac2_code8_cnt    " },
    { VDPU_AC2_CODE7_CNT     , 20,  8, 16, "sw20_ac2_code7_cnt    " },
    { VDPU_AC2_CODE6_CNT     , 20,  7,  8, "sw20_ac2_code6_cnt    " },
    { VDPU_AC2_CODE5_CNT     , 20,  6,  0, "sw20_ac2_code5_cnt    " },
    { VDPU_SCAN_MAP_6        , 20,  6, 24, "sw20_scan_map_6       " },
    { VDPU_SCAN_MAP_7        , 20,  6, 18, "sw20_scan_map_7       " },
    { VDPU_SCAN_MAP_8        , 20,  6, 12, "sw20_scan_map_8       " },
    { VDPU_SCAN_MAP_9        , 20,  6,  6, "sw20_scan_map_9       " },
    { VDPU_SCAN_MAP_10       , 20,  6,  0, "sw20_scan_map_10      " },
    { VDPU_REFER7_BASE       , 21, 32,  0, "sw21_refer7_base      " },
    { VDPU_REFER7_FIELD_E    , 21,  1,  1, "sw21_refer7_field_e   " },
    { VDPU_REFER7_TOPC_E     , 21,  1,  0, "sw21_refer7_topc_e    " },
    { VDPU_TRB_PER_TRD_D1    , 21, 27,  0, "sw21_trb_per_trd_d1   " },
    { VDPU_AC2_CODE12_CNT    , 21,  8, 24, "sw21_ac2_code12_cnt   " },
    { VDPU_AC2_CODE11_CNT    , 21,  8, 16, "sw21_ac2_code11_cnt   " },
    { VDPU_AC2_CODE10_CNT    , 21,  8,  8, "sw21_ac2_code10_cnt   " },
    { VDPU_AC2_CODE9_CNT     , 21,  8,  0, "sw21_ac2_code9_cnt    " },
    { VDPU_SCAN_MAP_11       , 21,  6, 24, "sw21_scan_map_11      " },
    { VDPU_SCAN_MAP_12       , 21,  6, 18, "sw21_scan_map_12      " },
    { VDPU_SCAN_MAP_13       , 21,  6, 12, "sw21_scan_map_13      " },
    { VDPU_SCAN_MAP_14       , 21,  6,  6, "sw21_scan_map_14      " },
    { VDPU_SCAN_MAP_15       , 21,  6,  0, "sw21_scan_map_15      " },
    { VDPU_REFER8_BASE       , 22, 32,  0, "sw22_refer8_base      " },
    { VDPU_DCT_STRM1_BASE    , 22, 32,  0, "sw22_dct_strm1_base   " },
    { VDPU_REFER8_FIELD_E    , 22,  1,  1, "sw22_refer8_field_e   " },
    { VDPU_REFER8_TOPC_E     , 22,  1,  0, "sw22_refer8_topc_e    " },
    { VDPU_AC2_CODE16_CNT    , 22,  8, 24, "sw22_ac2_code16_cnt   " },
    { VDPU_AC2_CODE15_CNT    , 22,  8, 16, "sw22_ac2_code15_cnt   " },
    { VDPU_AC2_CODE14_CNT    , 22,  8,  8, "sw22_ac2_code14_cnt   " },
    { VDPU_AC2_CODE13_CNT    , 22,  8,  0, "sw22_ac2_code13_cnt   " },
    { VDPU_SCAN_MAP_16       , 22,  6, 24, "sw22_scan_map_16      " },
    { VDPU_SCAN_MAP_17       , 22,  6, 18, "sw22_scan_map_17      " },
    { VDPU_SCAN_MAP_18       , 22,  6, 12, "sw22_scan_map_18      " },
    { VDPU_SCAN_MAP_19       , 22,  6,  6, "sw22_scan_map_19      " },
    { VDPU_SCAN_MAP_20       , 22,  6,  0, "sw22_scan_map_20      " },
    { VDPU_REFER9_BASE       , 23, 32,  0, "sw23_refer9_base      " },
    { VDPU_DCT_STRM2_BASE    , 23, 32,  0, "sw23_dct_strm2_base   " },
    { VDPU_REFER9_FIELD_E    , 23,  1,  1, "sw23_refer9_field_e   " },
    { VDPU_REFER9_TOPC_E     , 23,  1,  0, "sw23_refer9_topc_e    " },
    { VDPU_DC1_CODE8_CNT     , 23,  4, 28, "sw23_dc1_code8_cnt    " },
    { VDPU_DC1_CODE7_CNT     , 23,  4, 24, "sw23_dc1_code7_cnt    " },
    { VDPU_DC1_CODE6_CNT     , 23,  4, 20, "sw23_dc1_code6_cnt    " },
    { VDPU_DC1_CODE5_CNT     , 23,  4, 16, "sw23_dc1_code5_cnt    " },
    { VDPU_DC1_CODE4_CNT     , 23,  4, 12, "sw23_dc1_code4_cnt    " },
    { VDPU_DC1_CODE3_CNT     , 23,  4,  8, "sw23_dc1_code3_cnt    " },
    { VDPU_DC1_CODE2_CNT     , 23,  3,  4, "sw23_dc1_code2_cnt    " },
    { VDPU_DC1_CODE1_CNT     , 23,  2,  0, "sw23_dc1_code1_cnt    " },
    { VDPU_SCAN_MAP_21       , 23,  6, 24, "sw23_scan_map_21      " },
    { VDPU_SCAN_MAP_22       , 23,  6, 18, "sw23_scan_map_22      " },
    { VDPU_SCAN_MAP_23       , 23,  6, 12, "sw23_scan_map_23      " },
    { VDPU_SCAN_MAP_24       , 23,  6,  6, "sw23_scan_map_24      " },
    { VDPU_SCAN_MAP_25       , 23,  6,  0, "sw23_scan_map_25      " },
    { VDPU_REFER10_BASE      , 24, 32,  0, "sw24_refer10_base     " },
    { VDPU_DCT_STRM3_BASE    , 24, 32,  0, "sw24_dct_strm3_base   " },
    { VDPU_REFER10_FIELD_E   , 24,  1,  1, "sw24_refer10_field_e  " },
    { VDPU_REFER10_TOPC_E    , 24,  1,  0, "sw24_refer10_topc_e   " },
    { VDPU_DC1_CODE16_CNT    , 24,  4, 28, "sw24_dc1_code16_cnt   " },
    { VDPU_DC1_CODE15_CNT    , 24,  4, 24, "sw24_dc1_code15_cnt   " },
    { VDPU_DC1_CODE14_CNT    , 24,  4, 20, "sw24_dc1_code14_cnt   " },
    { VDPU_DC1_CODE13_CNT    , 24,  4, 16, "sw24_dc1_code13_cnt   " },
    { VDPU_DC1_CODE12_CNT    , 24,  4, 12, "sw24_dc1_code12_cnt   " },
    { VDPU_DC1_CODE11_CNT    , 24,  4,  8, "sw24_dc1_code11_cnt   " },
    { VDPU_DC1_CODE10_CNT    , 24,  4,  4, "sw24_dc1_code10_cnt   " },
    { VDPU_DC1_CODE9_CNT     , 24,  4,  0, "sw24_dc1_code9_cnt    " },
    { VDPU_SCAN_MAP_26       , 24,  6, 24, "sw24_scan_map_26      " },
    { VDPU_SCAN_MAP_27       , 24,  6, 18, "sw24_scan_map_27      " },
    { VDPU_SCAN_MAP_28       , 24,  6, 12, "sw24_scan_map_28      " },
    { VDPU_SCAN_MAP_29       , 24,  6,  6, "sw24_scan_map_29      " },
    { VDPU_SCAN_MAP_30       , 24,  6,  0, "sw24_scan_map_30      " },
    { VDPU_REFER11_BASE      , 25, 32,  0, "sw25_refer11_base     " },
    { VDPU_DCT_STRM4_BASE    , 25, 32,  0, "sw25_dct_strm4_base   " },
    { VDPU_REFER11_FIELD_E   , 25,  1,  1, "sw25_refer11_field_e  " },
    { VDPU_REFER11_TOPC_E    , 25,  1,  0, "sw25_refer11_topc_e   " },
    { VDPU_DC2_CODE8_CNT     , 25,  4, 28, "sw25_dc2_code8_cnt    " },
    { VDPU_DC2_CODE7_CNT     , 25,  4, 24, "sw25_dc2_code7_cnt    " },
    { VDPU_DC2_CODE6_CNT     , 25,  4, 20, "sw25_dc2_code6_cnt    " },
    { VDPU_DC2_CODE5_CNT     , 25,  4, 16, "sw25_dc2_code5_cnt    " },
    { VDPU_DC2_CODE4_CNT     , 25,  4, 12, "sw25_dc2_code4_cnt    " },
    { VDPU_DC2_CODE3_CNT     , 25,  4,  8, "sw25_dc2_code3_cnt    " },
    { VDPU_DC2_CODE2_CNT     , 25,  3,  4, "sw25_dc2_code2_cnt    " },
    { VDPU_DC2_CODE1_CNT     , 25,  2,  0, "sw25_dc2_code1_cnt    " },
    { VDPU_SCAN_MAP_31       , 25,  6, 24, "sw25_scan_map_31      " },
    { VDPU_SCAN_MAP_32       , 25,  6, 18, "sw25_scan_map_32      " },
    { VDPU_SCAN_MAP_33       , 25,  6, 12, "sw25_scan_map_33      " },
    { VDPU_SCAN_MAP_34       , 25,  6,  6, "sw25_scan_map_34      " },
    { VDPU_SCAN_MAP_35       , 25,  6,  0, "sw25_scan_map_35      " },
    { VDPU_REFER12_BASE      , 26, 32,  0, "sw26_refer12_base     " },
    { VDPU_DCT_STRM5_BASE    , 26, 32,  0, "sw26_dct_strm5_base   " },
    { VDPU_REFER12_FIELD_E   , 26,  1,  1, "sw26_refer12_field_e  " },
    { VDPU_REFER12_TOPC_E    , 26,  1,  0, "sw26_refer12_topc_e   " },
    { VDPU_DC2_CODE16_CNT    , 26,  4, 28, "sw26_dc2_code16_cnt   " },
    { VDPU_DC2_CODE15_CNT    , 26,  4, 24, "sw26_dc2_code15_cnt   " },
    { VDPU_DC2_CODE14_CNT    , 26,  4, 20, "sw26_dc2_code14_cnt   " },
    { VDPU_DC2_CODE13_CNT    , 26,  4, 16, "sw26_dc2_code13_cnt   " },
    { VDPU_DC2_CODE12_CNT    , 26,  4, 12, "sw26_dc2_code12_cnt   " },
    { VDPU_DC2_CODE11_CNT    , 26,  4,  8, "sw26_dc2_code11_cnt   " },
    { VDPU_DC2_CODE10_CNT    , 26,  4,  4, "sw26_dc2_code10_cnt   " },
    { VDPU_DC2_CODE9_CNT     , 26,  4,  0, "sw26_dc2_code9_cnt    " },
    { VDPU_SCAN_MAP_36       , 26,  6, 24, "sw26_scan_map_36      " },
    { VDPU_SCAN_MAP_37       , 26,  6, 18, "sw26_scan_map_37      " },
    { VDPU_SCAN_MAP_38       , 26,  6, 12, "sw26_scan_map_38      " },
    { VDPU_SCAN_MAP_39       , 26,  6,  6, "sw26_scan_map_39      " },
    { VDPU_SCAN_MAP_40       , 26,  6,  0, "sw26_scan_map_40      " },
    { VDPU_REFER13_BASE      , 27, 32,  0, "sw27_refer13_base     " },
    { VDPU_REFER13_FIELD_E   , 27,  1,  1, "sw27_refer13_field_e  " },
    { VDPU_REFER13_TOPC_E    , 27,  1,  0, "sw27_refer13_topc_e   " },
    { VDPU_DC3_CODE8_CNT     , 27,  4, 28, "sw27_dc3_code8_cnt    " },
    { VDPU_DC3_CODE7_CNT     , 27,  4, 24, "sw27_dc3_code7_cnt    " },
    { VDPU_DC3_CODE6_CNT     , 27,  4, 20, "sw27_dc3_code6_cnt    " },
    { VDPU_DC3_CODE5_CNT     , 27,  4, 16, "sw27_dc3_code5_cnt    " },
    { VDPU_DC3_CODE4_CNT     , 27,  4, 12, "sw27_dc3_code4_cnt    " },
    { VDPU_DC3_CODE3_CNT     , 27,  4,  8, "sw27_dc3_code3_cnt    " },
    { VDPU_DC3_CODE2_CNT     , 27,  3,  4, "sw27_dc3_code2_cnt    " },
    { VDPU_DC3_CODE1_CNT     , 27,  2,  0, "sw27_dc3_code1_cnt    " },
    { VDPU_BITPL_CTRL_BASE   , 27, 32,  0, "sw27_bitpl_ctrl_base  " },
    { VDPU_REFER14_BASE      , 28, 32,  0, "sw28_refer14_base     " },
    { VDPU_DCT_STRM6_BASE    , 28, 32,  0, "sw28_dct_strm6_base   " },
    { VDPU_REFER14_FIELD_E   , 28,  1,  1, "sw28_refer14_field_e  " },
    { VDPU_REFER14_TOPC_E    , 28,  1,  0, "sw28_refer14_topc_e   " },
    { VDPU_REF_INVD_CUR_1    , 28, 16, 16, "sw28_ref_invd_cur_1   " },
    { VDPU_REF_INVD_CUR_0    , 28, 16,  0, "sw28_ref_invd_cur_0   " },
    { VDPU_DC3_CODE16_CNT    , 28,  4, 28, "sw28_dc3_code16_cnt   " },
    { VDPU_DC3_CODE15_CNT    , 28,  4, 24, "sw28_dc3_code15_cnt   " },
    { VDPU_DC3_CODE14_CNT    , 28,  4, 20, "sw28_dc3_code14_cnt   " },
    { VDPU_DC3_CODE13_CNT    , 28,  4, 16, "sw28_dc3_code13_cnt   " },
    { VDPU_DC3_CODE12_CNT    , 28,  4, 12, "sw28_dc3_code12_cnt   " },
    { VDPU_DC3_CODE11_CNT    , 28,  4,  8, "sw28_dc3_code11_cnt   " },
    { VDPU_DC3_CODE10_CNT    , 28,  4,  4, "sw28_dc3_code10_cnt   " },
    { VDPU_DC3_CODE9_CNT     , 28,  4,  0, "sw28_dc3_code9_cnt    " },
    { VDPU_SCAN_MAP_41       , 28,  6, 24, "sw28_scan_map_41      " },
    { VDPU_SCAN_MAP_42       , 28,  6, 18, "sw28_scan_map_42      " },
    { VDPU_SCAN_MAP_43       , 28,  6, 12, "sw28_scan_map_43      " },
    { VDPU_SCAN_MAP_44       , 28,  6,  6, "sw28_scan_map_44      " },
    { VDPU_SCAN_MAP_45       , 28,  6,  0, "sw28_scan_map_45      " },
    { VDPU_REFER15_BASE      , 29, 32,  0, "sw29_refer15_base     " },
    { VDPU_INTER_VIEW_BASE   , 29, 32,  0, "sw29_inter_view_base  " },
    { VDPU_DCT_STRM7_BASE    , 29, 32,  0, "sw29_dct_strm7_base   " },
    { VDPU_REFER15_FIELD_E   , 29,  1,  1, "sw29_refer15_field_e  " },
    { VDPU_REFER15_TOPC_E    , 29,  1,  0, "sw29_refer15_topc_e   " },
    { VDPU_REF_INVD_CUR_3    , 29, 16, 16, "sw29_ref_invd_cur_3   " },
    { VDPU_REF_INVD_CUR_2    , 29, 16,  0, "sw29_ref_invd_cur_2   " },
    { VDPU_SCAN_MAP_46       , 29,  6, 24, "sw29_scan_map_46      " },
    { VDPU_SCAN_MAP_47       , 29,  6, 18, "sw29_scan_map_47      " },
    { VDPU_SCAN_MAP_48       , 29,  6, 12, "sw29_scan_map_48      " },
    { VDPU_SCAN_MAP_49       , 29,  6,  6, "sw29_scan_map_49      " },
    { VDPU_SCAN_MAP_50       , 29,  6,  0, "sw29_scan_map_50      " },
    { VDPU_REFER1_NBR        , 30, 16, 16, "sw30_refer1_nbr       " },
    { VDPU_REFER0_NBR        , 30, 16,  0, "sw30_refer0_nbr       " },
    { VDPU_REF_DIST_CUR_1    , 30, 16, 16, "sw30_ref_dist_cur_1   " },
    { VDPU_REF_DIST_CUR_0    , 30, 16,  0, "sw30_ref_dist_cur_0   " },
    { VDPU_FILT_TYPE         , 30,  1, 31, "sw30_filt_type        " },
    { VDPU_FILT_SHARPNESS    , 30,  3, 28, "sw30_filt_sharpness   " },
    { VDPU_FILT_MB_ADJ_0     , 30,  7, 21, "sw30_filt_mb_adj_0    " },
    { VDPU_FILT_MB_ADJ_1     , 30,  7, 14, "sw30_filt_mb_adj_1    " },
    { VDPU_FILT_MB_ADJ_2     , 30,  7,  7, "sw30_filt_mb_adj_2    " },
    { VDPU_FILT_MB_ADJ_3     , 30,  7,  0, "sw30_filt_mb_adj_3    " },
    { VDPU_REFER3_NBR        , 31, 16, 16, "sw31_refer3_nbr       " },
    { VDPU_REFER2_NBR        , 31, 16,  0, "sw31_refer2_nbr       " },
    { VDPU_SCAN_MAP_51       , 31,  6, 24, "sw31_scan_map_51      " },
    { VDPU_SCAN_MAP_52       , 31,  6, 18, "sw31_scan_map_52      " },
    { VDPU_SCAN_MAP_53       , 31,  6, 12, "sw31_scan_map_53      " },
    { VDPU_SCAN_MAP_54       , 31,  6,  6, "sw31_scan_map_54      " },
    { VDPU_SCAN_MAP_55       , 31,  6,  0, "sw31_scan_map_55      " },
    { VDPU_REF_DIST_CUR_3    , 31, 16, 16, "sw31_ref_dist_cur_3   " },
    { VDPU_REF_DIST_CUR_2    , 31, 16,  0, "sw31_ref_dist_cur_2   " },
    { VDPU_FILT_REF_ADJ_0    , 31,  7, 21, "sw31_filt_ref_adj_0   " },
    { VDPU_FILT_REF_ADJ_1    , 31,  7, 14, "sw31_filt_ref_adj_1   " },
    { VDPU_FILT_REF_ADJ_2    , 31,  7,  7, "sw31_filt_ref_adj_2   " },
    { VDPU_FILT_REF_ADJ_3    , 31,  7,  0, "sw31_filt_ref_adj_3   " },
    { VDPU_REFER5_NBR        , 32, 16, 16, "sw32_refer5_nbr       " },
    { VDPU_REFER4_NBR        , 32, 16,  0, "sw32_refer4_nbr       " },
    { VDPU_SCAN_MAP_56       , 32,  6, 24, "sw32_scan_map_56      " },
    { VDPU_SCAN_MAP_57       , 32,  6, 18, "sw32_scan_map_57      " },
    { VDPU_SCAN_MAP_58       , 32,  6, 12, "sw32_scan_map_58      " },
    { VDPU_SCAN_MAP_59       , 32,  6,  6, "sw32_scan_map_59      " },
    { VDPU_SCAN_MAP_60       , 32,  6,  0, "sw32_scan_map_60      " },
    { VDPU_REF_INVD_COL_1    , 32, 16, 16, "sw32_ref_invd_col_1   " },
    { VDPU_REF_INVD_COL_0    , 32, 16,  0, "sw32_ref_invd_col_0   " },
    { VDPU_FILT_LEVEL_0      , 32,  6, 18, "sw32_filt_level_0     " },
    { VDPU_FILT_LEVEL_1      , 32,  6, 12, "sw32_filt_level_1     " },
    { VDPU_FILT_LEVEL_2      , 32,  6,  6, "sw32_filt_level_2     " },
    { VDPU_FILT_LEVEL_3      , 32,  6,  0, "sw32_filt_level_3     " },
    { VDPU_REFER7_NBR        , 33, 16, 16, "sw33_refer7_nbr       " },
    { VDPU_REFER6_NBR        , 33, 16,  0, "sw33_refer6_nbr       " },
    { VDPU_SCAN_MAP_61       , 33,  6, 24, "sw33_scan_map_61      " },
    { VDPU_SCAN_MAP_62       , 33,  6, 18, "sw33_scan_map_62      " },
    { VDPU_SCAN_MAP_63       , 33,  6, 12, "sw33_scan_map_63      " },
    { VDPU_REF_INVD_COL_3    , 33, 16, 16, "sw33_ref_invd_col_3   " },
    { VDPU_REF_INVD_COL_2    , 33, 16,  0, "sw33_ref_invd_col_2   " },
    { VDPU_QUANT_DELTA_0     , 33,  5, 27, "sw33_quant_delta_0    " },
    { VDPU_QUANT_DELTA_1     , 33,  5, 22, "sw33_quant_delta_1    " },
    { VDPU_QUANT_0           , 33, 11, 11, "sw33_quant_0          " },
    { VDPU_QUANT_1           , 33, 11,  0, "sw33_quant_1          " },
    { VDPU_REFER9_NBR        , 34, 16, 16, "sw34_refer9_nbr       " },
    { VDPU_REFER8_NBR        , 34, 16,  0, "sw34_refer8_nbr       " },
    { VDPU_PRED_BC_TAP_0_3   , 34, 10, 22, "sw34_pred_bc_tap_0_3  " },
    { VDPU_PRED_BC_TAP_1_0   , 34, 10, 12, "sw34_pred_bc_tap_1_0  " },
    { VDPU_PRED_BC_TAP_1_1   , 34, 10,  2, "sw34_pred_bc_tap_1_1  " },
    { VDPU_REFER11_NBR       , 35, 16, 16, "sw35_refer11_nbr      " },
    { VDPU_REFER10_NBR       , 35, 16,  0, "sw35_refer10_nbr      " },
    { VDPU_PRED_BC_TAP_1_2   , 35, 10, 22, "sw35_pred_bc_tap_1_2  " },
    { VDPU_PRED_BC_TAP_1_3   , 35, 10, 12, "sw35_pred_bc_tap_1_3  " },
    { VDPU_PRED_BC_TAP_2_0   , 35, 10,  2, "sw35_pred_bc_tap_2_0  " },
    { VDPU_REFER13_NBR       , 36, 16, 16, "sw36_refer13_nbr      " },
    { VDPU_REFER12_NBR       , 36, 16,  0, "sw36_refer12_nbr      " },
    { VDPU_PRED_BC_TAP_2_1   , 36, 10, 22, "sw36_pred_bc_tap_2_1  " },
    { VDPU_PRED_BC_TAP_2_2   , 36, 10, 12, "sw36_pred_bc_tap_2_2  " },
    { VDPU_PRED_BC_TAP_2_3   , 36, 10,  2, "sw36_pred_bc_tap_2_3  " },
    { VDPU_REFER15_NBR       , 37, 16, 16, "sw37_refer15_nbr      " },
    { VDPU_REFER14_NBR       , 37, 16,  0, "sw37_refer14_nbr      " },
    { VDPU_PRED_BC_TAP_3_0   , 37, 10, 22, "sw37_pred_bc_tap_3_0  " },
    { VDPU_PRED_BC_TAP_3_1   , 37, 10, 12, "sw37_pred_bc_tap_3_1  " },
    { VDPU_PRED_BC_TAP_3_2   , 37, 10,  2, "sw37_pred_bc_tap_3_2  " },
    { VDPU_REFER_LTERM_E     , 38, 32,  0, "sw38_refer_lterm_e    " },
    { VDPU_PRED_BC_TAP_3_3   , 38, 10, 22, "sw38_pred_bc_tap_3_3  " },
    { VDPU_PRED_BC_TAP_4_0   , 38, 10, 12, "sw38_pred_bc_tap_4_0  " },
    { VDPU_PRED_BC_TAP_4_1   , 38, 10,  2, "sw38_pred_bc_tap_4_1  " },
    { VDPU_REFER_VALID_E     , 39, 32,  0, "sw39_refer_valid_e    " },
    { VDPU_PRED_BC_TAP_4_2   , 39, 10, 22, "sw39_pred_bc_tap_4_2  " },
    { VDPU_PRED_BC_TAP_4_3   , 39, 10, 12, "sw39_pred_bc_tap_4_3  " },
    { VDPU_PRED_BC_TAP_5_0   , 39, 10,  2, "sw39_pred_bc_tap_5_0  " },
    { VDPU_QTABLE_BASE       , 40, 32,  0, "sw40_qtable_base      " },
    { VDPU_DIR_MV_BASE       , 41, 32,  0, "sw41_dir_mv_base      " },
    { VDPU_BINIT_RLIST_B2    , 42,  5, 25, "sw42_binit_rlist_b2   " },
    { VDPU_BINIT_RLIST_F2    , 42,  5, 20, "sw42_binit_rlist_f2   " },
    { VDPU_BINIT_RLIST_B1    , 42,  5, 15, "sw42_binit_rlist_b1   " },
    { VDPU_BINIT_RLIST_F1    , 42,  5, 10, "sw42_binit_rlist_f1   " },
    { VDPU_BINIT_RLIST_B0    , 42,  5,  5, "sw42_binit_rlist_b0   " },
    { VDPU_BINIT_RLIST_F0    , 42,  5,  0, "sw42_binit_rlist_f0   " },
    { VDPU_PRED_BC_TAP_5_1   , 42, 10, 22, "sw42_pred_bc_tap_5_1  " },
    { VDPU_PRED_BC_TAP_5_2   , 42, 10, 12, "sw42_pred_bc_tap_5_2  " },
    { VDPU_PRED_BC_TAP_5_3   , 42, 10,  2, "sw42_pred_bc_tap_5_3  " },
    { VDPU_PJPEG_DCCB_BASE   , 42, 32,  0, "sw42_pjpeg_dccb_base  " },
    { VDPU_BINIT_RLIST_B5    , 43,  5, 25, "sw43_binit_rlist_b5   " },
    { VDPU_BINIT_RLIST_F5    , 43,  5, 20, "sw43_binit_rlist_f5   " },
    { VDPU_BINIT_RLIST_B4    , 43,  5, 15, "sw43_binit_rlist_b4   " },
    { VDPU_BINIT_RLIST_F4    , 43,  5, 10, "sw43_binit_rlist_f4   " },
    { VDPU_BINIT_RLIST_B3    , 43,  5,  5, "sw43_binit_rlist_b3   " },
    { VDPU_BINIT_RLIST_F3    , 43,  5,  0, "sw43_binit_rlist_f3   " },
    { VDPU_PRED_BC_TAP_6_0   , 43, 10, 22, "sw43_pred_bc_tap_6_0  " },
    { VDPU_PRED_BC_TAP_6_1   , 43, 10, 12, "sw43_pred_bc_tap_6_1  " },
    { VDPU_PRED_BC_TAP_6_2   , 43, 10,  2, "sw43_pred_bc_tap_6_2  " },
    { VDPU_PJPEG_DCCR_BASE   , 43, 32,  0, "sw43_pjpeg_dccr_base  " },
    { VDPU_BINIT_RLIST_B8    , 44,  5, 25, "sw44_binit_rlist_b8   " },
    { VDPU_BINIT_RLIST_F8    , 44,  5, 20, "sw44_binit_rlist_f8   " },
    { VDPU_BINIT_RLIST_B7    , 44,  5, 15, "sw44_binit_rlist_b7   " },
    { VDPU_BINIT_RLIST_F7    , 44,  5, 10, "sw44_binit_rlist_f7   " },
    { VDPU_BINIT_RLIST_B6    , 44,  5,  5, "sw44_binit_rlist_b6   " },
    { VDPU_BINIT_RLIST_F6    , 44,  5,  0, "sw44_binit_rlist_f6   " },
    { VDPU_PRED_BC_TAP_6_3   , 44, 10, 22, "sw44_pred_bc_tap_6_3  " },
    { VDPU_PRED_BC_TAP_7_0   , 44, 10, 12, "sw44_pred_bc_tap_7_0  " },
    { VDPU_PRED_BC_TAP_7_1   , 44, 10,  2, "sw44_pred_bc_tap_7_1  " },
    { VDPU_BINIT_RLIST_B11   , 45,  5, 25, "sw45_binit_rlist_b11  " },
    { VDPU_BINIT_RLIST_F11   , 45,  5, 20, "sw45_binit_rlist_f11  " },
    { VDPU_BINIT_RLIST_B10   , 45,  5, 15, "sw45_binit_rlist_b10  " },
    { VDPU_BINIT_RLIST_F10   , 45,  5, 10, "sw45_binit_rlist_f10  " },
    { VDPU_BINIT_RLIST_B9    , 45,  5,  5, "sw45_binit_rlist_b9   " },
    { VDPU_BINIT_RLIST_F9    , 45,  5,  0, "sw45_binit_rlist_f9   " },
    { VDPU_PRED_BC_TAP_7_2   , 45, 10, 22, "sw45_pred_bc_tap_7_2  " },
    { VDPU_PRED_BC_TAP_7_3   , 45, 10, 12, "sw45_pred_bc_tap_7_3  " },
    { VDPU_PRED_TAP_2_M1     , 45,  2, 10, "sw45_pred_tap_2_m1    " },
    { VDPU_PRED_TAP_2_4      , 45,  2,  8, "sw45_pred_tap_2_4     " },
    { VDPU_PRED_TAP_4_M1     , 45,  2,  6, "sw45_pred_tap_4_m1    " },
    { VDPU_PRED_TAP_4_4      , 45,  2,  4, "sw45_pred_tap_4_4     " },
    { VDPU_PRED_TAP_6_M1     , 45,  2,  2, "sw45_pred_tap_6_m1    " },
    { VDPU_PRED_TAP_6_4      , 45,  2,  0, "sw45_pred_tap_6_4     " },
    { VDPU_BINIT_RLIST_B14   , 46,  5, 25, "sw46_binit_rlist_b14  " },
    { VDPU_BINIT_RLIST_F14   , 46,  5, 20, "sw46_binit_rlist_f14  " },
    { VDPU_BINIT_RLIST_B13   , 46,  5, 15, "sw46_binit_rlist_b13  " },
    { VDPU_BINIT_RLIST_F13   , 46,  5, 10, "sw46_binit_rlist_f13  " },
    { VDPU_BINIT_RLIST_B12   , 46,  5,  5, "sw46_binit_rlist_b12  " },
    { VDPU_BINIT_RLIST_F12   , 46,  5,  0, "sw46_binit_rlist_f12  " },
    { VDPU_QUANT_DELTA_2     , 46,  5, 27, "sw46_quant_delta_2    " },
    { VDPU_QUANT_DELTA_3     , 46,  5, 22, "sw46_quant_delta_3    " },
    { VDPU_QUANT_2           , 46, 11, 11, "sw46_quant_2          " },
    { VDPU_QUANT_3           , 46, 11,  0, "sw46_quant_3          " },
    { VDPU_PINIT_RLIST_F3    , 47,  5, 25, "sw47_pinit_rlist_f3   " },
    { VDPU_PINIT_RLIST_F2    , 47,  5, 20, "sw47_pinit_rlist_f2   " },
    { VDPU_PINIT_RLIST_F1    , 47,  5, 15, "sw47_pinit_rlist_f1   " },
    { VDPU_PINIT_RLIST_F0    , 47,  5, 10, "sw47_pinit_rlist_f0   " },
    { VDPU_BINIT_RLIST_B15   , 47,  5,  5, "sw47_binit_rlist_b15  " },
    { VDPU_BINIT_RLIST_F15   , 47,  5,  0, "sw47_binit_rlist_f15  " },
    { VDPU_QUANT_DELTA_4     , 47,  5, 27, "sw47_quant_delta_4    " },
    { VDPU_QUANT_4           , 47, 11, 11, "sw47_quant_4          " },
    { VDPU_QUANT_5           , 47, 11,  0, "sw47_quant_5          " },
    { VDPU_STARTMB_X         , 48,  9, 23, "sw48_startmb_x        " },
    { VDPU_STARTMB_Y         , 48,  8, 15, "sw48_startmb_y        " },
    { VDPU_PRED_BC_TAP_0_0   , 49, 10, 22, "sw49_pred_bc_tap_0_0  " },
    { VDPU_PRED_BC_TAP_0_1   , 49, 10, 12, "sw49_pred_bc_tap_0_1  " },
    { VDPU_PRED_BC_TAP_0_2   , 49, 10,  2, "sw49_pred_bc_tap_0_2  " },
    { VDPU_REFBU_E           , 51,  1, 31, "sw51_refbu_e          " },
    { VDPU_REFBU_THR         , 51, 12, 19, "sw51_refbu_thr        " },
    { VDPU_REFBU_PICID       , 51,  5, 14, "sw51_refbu_picid      " },
    { VDPU_REFBU_EVAL_E      , 51,  1, 13, "sw51_refbu_eval_e     " },
    { VDPU_REFBU_FPARMOD_E   , 51,  1, 12, "sw51_refbu_fparmod_e  " },
    { VDPU_REFBU_Y_OFFSET    , 51,  9,  0, "sw51_refbu_y_offset   " },
    { VDPU_REFBU_HIT_SUM     , 52, 16, 16, "sw52_refbu_hit_sum    " },
    { VDPU_REFBU_INTRA_SUM   , 52, 16,  0, "sw52_refbu_intra_sum  " },
    { VDPU_REFBU_Y_MV_SUM    , 53, 22,  0, "sw53_refbu_y_mv_sum   " },
    { VDPU_REFBU2_BUF_E      , 55,  1, 31, "sw55_refbu2_buf_e     " },
    { VDPU_REFBU2_THR        , 55, 12, 19, "sw55_refbu2_thr       " },
    { VDPU_REFBU2_PICID      , 55,  5, 14, "sw55_refbu2_picid     " },
    { VDPU_APF_THRESHOLD     , 55, 14,  0, "sw55_apf_threshold    " },
    { VDPU_REFBU_TOP_SUM     , 56, 16, 16, "sw56_refbu_top_sum    " },
    { VDPU_REFBU_BOT_SUM     , 56, 16,  0, "sw56_refbu_bot_sum    " },
    { VDPU_DEC_CH8PIX_BASE   , 59, 32,  0, "sw59_dec_ch8pix_base  " },
    { VDPU_PP_BUS_INT        , 60,  1, 13, "sw60_pp_bus_int       " },
    { VDPU_PP_RDY_INT        , 60,  1, 12, "sw60_pp_rdy_int       " },
    { VDPU_PP_IRQ            , 60,  1,  8, "sw60_pp_irq           " },
    { VDPU_PP_IRQ_DIS        , 60,  1,  4, "sw60_pp_irq_dis       " },
    { VDPU_PP_PIPELINE_E     , 60,  1,  1, "sw60_pp_pipeline_e    " },
    { VDPU_PP_E              , 60,  1,  0, "sw60_pp_e             " },
    { VDPU_PP_AXI_RD_ID      , 61,  8, 24, "sw61_pp_axi_rd_id     " },
    { VDPU_PP_AXI_WR_ID      , 61,  8, 16, "sw61_pp_axi_wr_id     " },
    { VDPU_PP_AHB_HLOCK_E    , 61,  1, 15, "sw61_pp_ahb_hlock_e   " },
    { VDPU_PP_SCMD_DIS       , 61,  1, 14, "sw61_pp_scmd_dis      " },
    { VDPU_PP_IN_A2_ENDSEL   , 61,  1, 13, "sw61_pp_in_a2_endsel  " },
    { VDPU_PP_IN_A1_SWAP32   , 61,  1, 12, "sw61_pp_in_a1_swap32  " },
    { VDPU_PP_IN_A1_ENDIAN   , 61,  1, 11, "sw61_pp_in_a1_endian  " },
    { VDPU_PP_IN_SWAP32_E    , 61,  1, 10, "sw61_pp_in_swap32_e   " },
    { VDPU_PP_DATA_DISC_E    , 61,  1,  9, "sw61_pp_data_disc_e   " },
    { VDPU_PP_CLK_GATE_E     , 61,  1,  8, "sw61_pp_clk_gate_e    " },
    { VDPU_PP_IN_ENDIAN      , 61,  1,  7, "sw61_pp_in_endian     " },
    { VDPU_PP_OUT_ENDIAN     , 61,  1,  6, "sw61_pp_out_endian    " },
    { VDPU_PP_OUT_SWAP32_E   , 61,  1,  5, "sw61_pp_out_swap32_e  " },
    { VDPU_PP_MAX_BURST      , 61,  5,  0, "sw61_pp_max_burst     " },
    { VDPU_DEINT_E           , 62,  1, 31, "sw62_deint_e          " },
    { VDPU_DEINT_THRESHOLD   , 62, 14, 16, "sw62_deint_threshold  " },
    { VDPU_DEINT_BLEND_E     , 62,  1, 15, "sw62_deint_blend_e    " },
    { VDPU_DEINT_EDGE_DET    , 62, 15,  0, "sw62_deint_edge_det   " },
    { VDPU_PP_IN_LU_BASE     , 63, 32,  0, "sw63_pp_in_lu_base    " },
    { VDPU_PP_IN_CB_BASE     , 64, 32,  0, "sw64_pp_in_cb_base    " },
    { VDPU_PP_IN_CR_BASE     , 65, 32,  0, "sw65_pp_in_cr_base    " },
    { VDPU_PP_OUT_LU_BASE    , 66, 32,  0, "sw66_pp_out_lu_base   " },
    { VDPU_PP_OUT_CH_BASE    , 67, 32,  0, "sw67_pp_out_ch_base   " },
    { VDPU_CONTRAST_THR1     , 68,  8, 24, "sw68_contrast_thr1    " },
    { VDPU_CONTRAST_OFF2     , 68, 10, 10, "sw68_contrast_off2    " },
    { VDPU_CONTRAST_OFF1     , 68, 10,  0, "sw68_contrast_off1    " },
    { VDPU_PP_IN_START_CH    , 69,  1, 31, "sw69_pp_in_start_ch   " },
    { VDPU_PP_IN_CR_FIRST    , 69,  1, 30, "sw69_pp_in_cr_first   " },
    { VDPU_PP_OUT_START_CH   , 69,  1, 29, "sw69_pp_out_start_ch  " },
    { VDPU_PP_OUT_CR_FIRST   , 69,  1, 28, "sw69_pp_out_cr_first  " },
    { VDPU_COLOR_COEFFA2     , 69, 10, 18, "sw69_color_coeffa2    " },
    { VDPU_COLOR_COEFFA1     , 69, 10,  8, "sw69_color_coeffa1    " },
    { VDPU_CONTRAST_THR2     , 69,  8,  0, "sw69_contrast_thr2    " },
    { VDPU_COLOR_COEFFD      , 70, 10, 20, "sw70_color_coeffd     " },
    { VDPU_COLOR_COEFFC      , 70, 10, 10, "sw70_color_coeffc     " },
    { VDPU_COLOR_COEFFB      , 70, 10,  0, "sw70_color_coeffb     " },
    { VDPU_CROP_STARTX       , 71,  9, 21, "sw71_crop_startx      " },
    { VDPU_ROTATION_MODE     , 71,  3, 18, "sw71_rotation_mode    " },
    { VDPU_COLOR_COEFFF      , 71,  8, 10, "sw71_color_coefff     " },
    { VDPU_COLOR_COEFFE      , 71, 10,  0, "sw71_color_coeffe     " },
    { VDPU_CROP_STARTY       , 72,  8, 24, "sw72_crop_starty      " },
    { VDPU_RANGEMAP_COEF_Y   , 72,  5, 18, "sw72_rangemap_coef_y  " },
    { VDPU_PP_IN_HEIGHT      , 72,  8,  9, "sw72_pp_in_height     " },
    { VDPU_PP_IN_WIDTH       , 72,  9,  0, "sw72_pp_in_width      " },
    { VDPU_PP_BOT_YIN_BASE   , 73, 32,  0, "sw73_pp_bot_yin_base  " },
    { VDPU_PP_BOT_CIN_BASE   , 74, 32,  0, "sw74_pp_bot_cin_base  " },
    { VDPU_RANGEMAP_Y_E      , 79,  1, 31, "sw79_rangemap_y_e     " },
    { VDPU_RANGEMAP_C_E      , 79,  1, 30, "sw79_rangemap_c_e     " },
    { VDPU_YCBCR_RANGE       , 79,  1, 29, "sw79_ycbcr_range      " },
    { VDPU_RGB_PIX_IN32      , 79,  1, 28, "sw79_rgb_pix_in32     " },
    { VDPU_RGB_R_PADD        , 79,  5, 23, "sw79_rgb_r_padd       " },
    { VDPU_RGB_G_PADD        , 79,  5, 18, "sw79_rgb_g_padd       " },
    { VDPU_SCALE_WRATIO      , 79, 18,  0, "sw79_scale_wratio     " },
    { VDPU_PP_IN_STRUCT      , 80,  3, 27, "sw80_pp_in_struct     " },
    { VDPU_HOR_SCALE_MODE    , 80,  2, 25, "sw80_hor_scale_mode   " },
    { VDPU_VER_SCALE_MODE    , 80,  2, 23, "sw80_ver_scale_mode   " },
    { VDPU_RGB_B_PADD        , 80,  5, 18, "sw80_rgb_b_padd       " },
    { VDPU_SCALE_HRATIO      , 80, 18,  0, "sw80_scale_hratio     " },
    { VDPU_WSCALE_INVRA      , 81, 16, 16, "sw81_wscale_invra     " },
    { VDPU_HSCALE_INVRA      , 81, 16,  0, "sw81_hscale_invra     " },
    { VDPU_R_MASK            , 82, 32,  0, "sw82_r_mask           " },
    { VDPU_G_MASK            , 83, 32,  0, "sw83_g_mask           " },
    { VDPU_B_MASK            , 84, 32,  0, "sw84_b_mask           " },
    { VDPU_PP_IN_FORMAT      , 85,  3, 29, "sw85_pp_in_format     " },
    { VDPU_PP_OUT_FORMAT     , 85,  3, 26, "sw85_pp_out_format    " },
    { VDPU_PP_OUT_HEIGHT     , 85, 11, 15, "sw85_pp_out_height    " },
    { VDPU_PP_OUT_WIDTH      , 85, 11,  4, "sw85_pp_out_width     " },
    { VDPU_PP_OUT_SWAP16_E   , 85,  1,  2, "sw85_pp_out_swap16_e  " },
    { VDPU_PP_CROP8_R_E      , 85,  1,  1, "sw85_pp_crop8_r_e     " },
    { VDPU_PP_CROP8_D_E      , 85,  1,  0, "sw85_pp_crop8_d_e     " },
    { VDPU_PP_IN_FORMAT_ES   , 86,  3, 29, "sw86_pp_in_format_es  " },
    { VDPU_RANGEMAP_COEF_C   , 86,  5, 23, "sw86_rangemap_coef_c  " },
    { VDPU_MASK1_ABLEND_E    , 86,  1, 22, "sw86_mask1_ablend_e   " },
    { VDPU_MASK1_STARTY      , 86, 11, 11, "sw86_mask1_starty     " },
    { VDPU_MASK1_STARTX      , 86, 11,  0, "sw86_mask1_startx     " },
    { VDPU_MASK2_ABLEND_E    , 87,  1, 22, "sw87_mask2_ablend_e   " },
    { VDPU_MASK2_STARTY      , 87, 11, 11, "sw87_mask2_starty     " },
    { VDPU_MASK2_STARTX      , 87, 11,  0, "sw87_mask2_startx     " },
    { VDPU_EXT_ORIG_WIDTH    , 88,  9, 23, "sw88_ext_orig_width   " },
    { VDPU_MASK1_E           , 88,  1, 22, "sw88_mask1_e          " },
    { VDPU_MASK1_ENDY        , 88, 11, 11, "sw88_mask1_endy       " },
    { VDPU_MASK1_ENDX        , 88, 11,  0, "sw88_mask1_endx       " },
    { VDPU_MASK2_E           , 89,  1, 22, "sw89_mask2_e          " },
    { VDPU_MASK2_ENDY        , 89, 11, 11, "sw89_mask2_endy       " },
    { VDPU_MASK2_ENDX        , 89, 11,  0, "sw89_mask2_endx       " },
    { VDPU_RIGHT_CROSS_E     , 90,  1, 29, "sw90_right_cross_e    " },
    { VDPU_LEFT_CROSS_E      , 90,  1, 28, "sw90_left_cross_e     " },
    { VDPU_UP_CROSS_E        , 90,  1, 27, "sw90_up_cross_e       " },
    { VDPU_DOWN_CROSS_E      , 90,  1, 26, "sw90_down_cross_e     " },
    { VDPU_UP_CROSS          , 90, 11, 15, "sw90_up_cross         " },
    { VDPU_DOWN_CROSS        , 90, 11,  0, "sw90_down_cross       " },
    { VDPU_DITHER_SELECT_R   , 91,  2, 30, "sw91_dither_select_r  " },
    { VDPU_DITHER_SELECT_G   , 91,  2, 28, "sw91_dither_select_g  " },
    { VDPU_DITHER_SELECT_B   , 91,  2, 26, "sw91_dither_select_b  " },
    { VDPU_RIGHT_CROSS       , 91, 11, 11, "sw91_right_cross      " },
    { VDPU_LEFT_CROSS        , 91, 11,  0, "sw91_left_cross       " },
    { VDPU_PP_IN_H_EXT       , 92,  3, 29, "sw92_pp_in_h_ext      " },
    { VDPU_PP_IN_W_EXT       , 92,  3, 26, "sw92_pp_in_w_ext      " },
    { VDPU_CROP_STARTY_EXT   , 92,  3, 23, "sw92_crop_starty_ext  " },
    { VDPU_CROP_STARTX_EXT   , 92,  3, 20, "sw92_crop_startx_ext  " },
    { VDPU_DISPLAY_WIDTH     , 92, 12,  0, "sw92_display_width    " },
    { VDPU_ABLEND1_BASE      , 93, 32,  0, "sw93_ablend1_base     " },
    { VDPU_ABLEND2_BASE      , 94, 32,  0, "sw94_ablend2_base     " },
    { VDPU_DEC_IRQ_STAT      ,  1,  7, 12, "sw01_dec_irq_stat     " },

    { VDPU_MAX_SIZE          , DEC_X170_REGISTERS,  0,  0, "vdpu_max_size         " },

};


const RK_U32 g_refPicNum[16] = {
    VDPU_REFER0_NBR,  VDPU_REFER1_NBR,  VDPU_REFER2_NBR,
    VDPU_REFER3_NBR,  VDPU_REFER4_NBR,  VDPU_REFER5_NBR,
    VDPU_REFER6_NBR,  VDPU_REFER7_NBR,  VDPU_REFER8_NBR,
    VDPU_REFER9_NBR,  VDPU_REFER10_NBR, VDPU_REFER11_NBR,
    VDPU_REFER12_NBR, VDPU_REFER13_NBR, VDPU_REFER14_NBR,
    VDPU_REFER15_NBR
};
const RK_U32 g_refPicList0[16] = {
    VDPU_BINIT_RLIST_F0,  VDPU_BINIT_RLIST_F1,  VDPU_BINIT_RLIST_F2,
    VDPU_BINIT_RLIST_F3,  VDPU_BINIT_RLIST_F4,  VDPU_BINIT_RLIST_F5,
    VDPU_BINIT_RLIST_F6,  VDPU_BINIT_RLIST_F7,  VDPU_BINIT_RLIST_F8,
    VDPU_BINIT_RLIST_F9,  VDPU_BINIT_RLIST_F10, VDPU_BINIT_RLIST_F11,
    VDPU_BINIT_RLIST_F12, VDPU_BINIT_RLIST_F13, VDPU_BINIT_RLIST_F14,
    VDPU_BINIT_RLIST_F15
};

const RK_U32 g_refPicList1[16] = {
    VDPU_BINIT_RLIST_B0,  VDPU_BINIT_RLIST_B1,  VDPU_BINIT_RLIST_B2,
    VDPU_BINIT_RLIST_B3,  VDPU_BINIT_RLIST_B4,  VDPU_BINIT_RLIST_B5,
    VDPU_BINIT_RLIST_B6,  VDPU_BINIT_RLIST_B7,  VDPU_BINIT_RLIST_B8,
    VDPU_BINIT_RLIST_B9,  VDPU_BINIT_RLIST_B10, VDPU_BINIT_RLIST_B11,
    VDPU_BINIT_RLIST_B12, VDPU_BINIT_RLIST_B13, VDPU_BINIT_RLIST_B14,
    VDPU_BINIT_RLIST_B15
};

const RK_U32 g_refPicListP[16] = {
    VDPU_PINIT_RLIST_F0,  VDPU_PINIT_RLIST_F1,  VDPU_PINIT_RLIST_F2,
    VDPU_PINIT_RLIST_F3,  VDPU_PINIT_RLIST_F4,  VDPU_PINIT_RLIST_F5,
    VDPU_PINIT_RLIST_F6,  VDPU_PINIT_RLIST_F7,  VDPU_PINIT_RLIST_F8,
    VDPU_PINIT_RLIST_F9,  VDPU_PINIT_RLIST_F10, VDPU_PINIT_RLIST_F11,
    VDPU_PINIT_RLIST_F12, VDPU_PINIT_RLIST_F13, VDPU_PINIT_RLIST_F14,
    VDPU_PINIT_RLIST_F15
};

const RK_U32 g_ValueList0[34] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};

const RK_U32 g_ValueList1[34] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};

const RK_U32 g_ValueListP[34] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};

const RK_U32 g_refBase[16] = {
    VDPU_REFER0_BASE,  VDPU_REFER1_BASE,  VDPU_REFER2_BASE,
    VDPU_REFER3_BASE,  VDPU_REFER4_BASE,  VDPU_REFER5_BASE,
    VDPU_REFER6_BASE,  VDPU_REFER7_BASE,  VDPU_REFER8_BASE,
    VDPU_REFER9_BASE,  VDPU_REFER10_BASE, VDPU_REFER11_BASE,
    VDPU_REFER12_BASE, VDPU_REFER13_BASE, VDPU_REFER14_BASE,
    VDPU_REFER15_BASE
};


#ifndef ANDROID
static RK_S32 VPUClientGetIOMMUStatus()
{
    return 0;
}
#endif


static RK_U32 check_dpb_buffer_is_valid(H264dHalCtx_t *p_hal, RK_U32 dpb_idx)
{

    (void)p_hal;
    (void)dpb_idx;

    return RET_TURE;
}




/*!
***********************************************************************
* \brief
*    set picture parameter and set register
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_set_pic_regs(void *hal, HalRegDrvCtx_t *p_drv)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    RK_U32 mb_width = 0, mb_height = 0;

    mb_width  = p_hal->pp->wFrameWidthInMbsMinus1 + 1;
    mb_height = (2 - p_hal->pp->frame_mbs_only_flag) * (p_hal->pp->wFrameHeightInMbsMinus1 + 1);

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PIC_MB_WIDTH,    mb_width));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PIC_MB_HEIGHT_P, mb_height));

    return MPP_OK;
__FAILED:
    return ret;
}

/*!
***********************************************************************
* \brief
*    set vlc register
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_set_vlc_regs(void *hal, HalRegDrvCtx_t *p_drv)
{
    RK_U32 i = 0;
    RK_U32 *pocBase = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;
    MppBuffer bitstream_buf = NULL;
    RK_U32 validTmp = 0, validFlags = 0;
    RK_U32 longTermTmp = 0, longTermflags = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_OUT_DIS    , 0));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_RLC_MODE_E     , 0));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_INIT_QP        , p_hal->pp->pic_init_qp_minus26 + 26));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REFIDX0_ACTIVE , p_hal->pp->num_ref_idx_l0_active_minus1 + 1));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REF_FRAMES     , p_hal->pp->num_ref_frames));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_FRAMENUM_LEN   , p_hal->pp->log2_max_frame_num_minus4 + 4));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_FRAMENUM       , p_hal->pp->frame_num));

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_CONST_INTRA_E  , p_hal->pp->constrained_intra_pred_flag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_FILT_CTRL_PRES , p_hal->pp->deblocking_filter_control_present_flag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_RDPIC_CNT_PRES , p_hal->pp->redundant_pic_cnt_present_flag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REFPIC_MK_LEN  , p_hal->slice_long->drpm_used_bitlen));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_IDR_PIC_E      , p_hal->slice_long->idr_flag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_IDR_PIC_ID     , p_hal->slice_long->idr_pic_id));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PPS_ID         , p_hal->slice_long->active_pps_id));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_POC_LENGTH     , p_hal->slice_long->poc_used_bitlen));

    /* reference picture flags, TODO separate fields */
    if (p_hal->pp->field_pic_flag) {
        for (i = 0; i < 32; i++) {
            if (p_hal->pp->RefFrameList[i / 2].bPicEntry == 0xff) { //!< invalid
                longTermflags <<= 1;
                validFlags    <<= 1;
            } else {
                longTermTmp = p_hal->pp->RefFrameList[i / 2].AssociatedFlag; //!< get long term flag
                longTermflags = (longTermflags << 1) | longTermTmp;

                validTmp = check_dpb_buffer_is_valid(p_hal, p_hal->pp->RefFrameList[i / 2].Index7Bits);
                validTmp = validTmp && ((p_hal->pp->UsedForReferenceFlags >> i) & 0x01);
                validFlags = (validFlags << 1) | validTmp;
            }
        }
        FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REFER_LTERM_E, longTermflags));
        FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REFER_VALID_E, validFlags));
    } else {
        for (i = 0; i < 16; i++) {
            if (p_hal->pp->RefFrameList[i].bPicEntry == 0xff) {  //!< invalid
                longTermflags <<= 1;
                validFlags    <<= 1;
            } else {
                longTermTmp = p_hal->pp->RefFrameList[i].AssociatedFlag;
                longTermflags = (longTermflags << 1) | longTermTmp;
                validTmp = check_dpb_buffer_is_valid(p_hal, p_hal->pp->RefFrameList[i].Index7Bits);
                validTmp = validTmp && ((p_hal->pp->UsedForReferenceFlags >> (2 * i)) & 0x03);
                validFlags = (validFlags << 1) | validTmp;
            }
        }
        FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REFER_LTERM_E, longTermflags << 16));
        FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REFER_VALID_E, validFlags    << 16));


		//FPRINT(g_debug_file0, "ref_valid_e=%08x, longterm=%08x, mvc_e=%d, cur_poc=%d \n", 
		//	validFlags << 16, longTermflags << 16, 0, p_hal->pp->CurrFieldOrderCnt[0]);

		//FPRINT(g_debug_file0, "--------- [FLUSH_CNT] flush framecnt=%d -------- \n", p_hal->iDecodedNum++);

    }

    for (i = 0; i < 16; i++) {
        if (p_hal->pp->RefFrameList[i].bPicEntry != 0xff) { //!< valid
            if (p_hal->pp->RefFrameList[i].AssociatedFlag) { //!< longterm flag
                FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refPicNum[i], p_hal->pp->LongTermPicNumList[i])); //!< pic_num
            } else {
                FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refPicNum[i], p_hal->pp->FrameNumList[i])); //< frame_num
            }
        }
    }
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PICORD_COUNT_E , 1));

    //!< set poc to buffer
    pocBase = (RK_U32 *) ((RK_U8 *)mpp_buffer_get_ptr(p_hal->cabac_buf) + VDPU_CABAC_TAB_SIZE);
    //!< set reference reorder poc
    for (i = 0; i < 32; i++) {
        if (p_hal->pp->RefFrameList[i / 2].bPicEntry != 0xff) {
            *pocBase++ = p_hal->pp->FieldOrderCntList[i / 2][i & 0x1];
        } else {
            *pocBase++ = 0;
        }
    }
    //!< set current poc
    if (p_hal->pp->field_pic_flag || !p_hal->pp->MbaffFrameFlag) {
        *pocBase++ = p_hal->pp->CurrFieldOrderCnt[0];
        *pocBase++ = p_hal->pp->CurrFieldOrderCnt[1];
    } else {
        *pocBase++ = p_hal->pp->CurrFieldOrderCnt[0];
        *pocBase++ = p_hal->pp->CurrFieldOrderCnt[1];
    }
#if 0

	//pocBase = (RK_U32 *) ((RK_U8 *)mpp_buffer_get_ptr(p_hal->cabac_buf) + VDPU_CABAC_TAB_SIZE);
	//for (i = 0; i < VDPU_POC_BUF_SIZE / 4; i++)
	//{
	//	FPRINT(g_debug_file1, "i=%d, poc_value=%d, idx=%d \n", i, *pocBase++, p_hal->pp->RefFrameList[i / 2].Index7Bits);
	//}
    //FPRINT(g_debug_file0, "------ g_framecnt=%d \n", p_hal->in_task->g_framecnt);


	pocBase = (RK_U32 *) ((RK_U8 *)mpp_buffer_get_ptr(p_hal->cabac_buf) + VDPU_CABAC_TAB_SIZE);
	for(i = 0; i < 16; i++) {
		RK_S32 dpb_idx = 0, longTermTmp = 0;
		if (p_hal->pp->RefFrameList[i / 2].bPicEntry != 0xff) {
			FPRINT(g_debug_file1, "i=%2d, picnum=%d, framenum=%2d,", i, p_hal->pp->FrameNumList[i], p_hal->pp->FrameNumList[i]);
			longTermTmp = p_hal->pp->RefFrameList[i].AssociatedFlag;
			dpb_idx = p_hal->pp->RefFrameList[i / 2].Index7Bits;
			FPRINT(g_debug_file1, " dbp_idx=%d, longterm=%d, poc=%d \n", dpb_idx, longTermTmp, *pocBase);
		}
		pocBase +=2;
	}
	hal_get_regdrv((HalRegDrvCtx_t *)p_hal->regs, VDPU_REFER_VALID_E, &validFlags);
	FPRINT(g_debug_file1, " view=%d, nonivref=%d, iv_base=%08x, ref_valid_e=%d, mvc_e=%d, cur_poc=%d \n", 0, 0, 0x0, validFlags, 0, *pocBase);
	FPRINT(g_debug_file1, "--------- [FLUSH_CNT] flush framecnt=%d --------\n", p_hal->iDecodedNum);
	 








#endif








    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_CABAC_E        , p_hal->pp->entropy_coding_mode_flag));
    //!< stream position update
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_START_CODE_E   , 1));
    mpp_buf_slot_get_prop(p_hal->packet_slots, p_hal->in_task->input, SLOT_BUFFER, &bitstream_buf);
    //!< p_hal->bitstream = (RK_U8 *)mpp_buffer_get_ptr(bitstream_buf); //!< get pointer from fd buffer
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_STRM_START_BIT , 0));    //!< sodb stream start bit
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_RLC_VLC_BASE   , mpp_buffer_get_fd(bitstream_buf)));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_STREAM_LEN     , p_hal->strm_len));

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);

    return MPP_OK;
__FAILED:
    return ret;
}

/*!
***********************************************************************
* \brief
*    set vlc reference
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_set_ref_regs(void *hal, HalRegDrvCtx_t *p_drv)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    DXVA_Slice_H264_Long *p_long = p_hal->slice_long;

    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    //!< P lists
    for (i = 0; i < MPP_ARRAY_ELEMS(g_refPicListP); i++) {
        if (p_long->RefPicList[0][i].bPicEntry == 0xff) {
            FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refPicListP[i], g_ValueListP[i]));
        } else {
            FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refPicListP[i], p_long->RefPicList[0][i].Index7Bits));
        }
    }
    //!< B list0
    for (i = 0; i < MPP_ARRAY_ELEMS(g_refPicList0); i++) {
        if (p_long->RefPicList[1][i].bPicEntry == 0xff) {
            FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refPicList0[i], g_ValueList0[i]));
        } else {
            FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refPicList0[i], p_long->RefPicList[1][i].Index7Bits));
        }
    }
    //!< B list1
    for (i = 0; i < MPP_ARRAY_ELEMS(g_refPicList1); i++) {
        if (p_long->RefPicList[2][i].bPicEntry == 0xff) {
            FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refPicList1[i], g_ValueList1[i]));
        } else {
            FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refPicList1[i], p_long->RefPicList[2][i].Index7Bits));
        }
    }
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);

    return MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    run Asic
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_set_asic_regs(void *hal, HalRegDrvCtx_t *p_drv)
{
    RK_U32 i = 0, j = 0;
    RK_U32 validTmp = 0;
    RK_U32 outPhyAddr = 0;
    RK_U32 dirMvOffset = 0;
    RK_U32 picSizeInMbs = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    MppBuffer frame_buf = NULL;
    

    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    DXVA_Slice_H264_Long *p_long = p_hal->slice_long;
    H264dHalOldDXVA_t *priv = (H264dHalOldDXVA_t *)p_hal->priv;

    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    /* reference picture physis address */
    for (i = 0, j = 0xff; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
        if (pp->RefFrameList[i].bPicEntry != 0xff) {
			mpp_buf_slot_get_prop(p_hal->frame_slots, pp->RefFrameList[i].Index7Bits, SLOT_BUFFER, &frame_buf); //!< reference phy addr
			j = i;
        } else/* if(j == 0xff)*/ {
			mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &frame_buf); //!< current out phy addr
		} 
		//else {
		//	mpp_buf_slot_get_prop(p_hal->frame_slots, j, SLOT_BUFFER, &frame_buf); //!< current out phy addr
		//}

        FUN_CHECK(ret = hal_set_regdrv(p_drv, g_refBase[i], mpp_buffer_get_fd(frame_buf)));
    }
    /* inter-view reference picture */
    if (pp->curr_layer_id && priv->ilt_dpb[0].valid /*pp->inter_view_flag*/) {
        mpp_buf_slot_get_prop(p_hal->frame_slots, priv->ilt_dpb[0].slot_index, SLOT_BUFFER, &frame_buf); //!< current out phy addr
        FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_INTER_VIEW_BASE, mpp_buffer_get_fd(frame_buf)));
        FUN_CHECK(ret = hal_get_regdrv(p_drv, VDPU_REFER_VALID_E, &validTmp));
        validTmp |= (pp->field_pic_flag ? 0x3 : 0x10000);
        FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REFER_VALID_E, validTmp)); //!< reference valid
    }
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_MVC_E, pp->curr_layer_id));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_FILTERING_DIS, 0)); //!< filterDisable = 0;
    mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &frame_buf); //!< current out phy addr
    outPhyAddr = mpp_buffer_get_fd(frame_buf);
    if (pp->field_pic_flag && pp->CurrPic.AssociatedFlag) {
        if (VPUClientGetIOMMUStatus() > 0) {
            outPhyAddr |= ((pp->wFrameWidthInMbsMinus1 + 1) * 16) << 10;
        } else {
            outPhyAddr +=  (pp->wFrameWidthInMbsMinus1 + 1) * 16;
        }
    }
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_OUT_BASE,  outPhyAddr)); //!< outPhyAddr, pp->CurrPic.Index7Bits

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_CH_QP_OFFSET,  pp->chroma_qp_index_offset));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_CH_QP_OFFSET2, pp->second_chroma_qp_index_offset));

    /* set default value for register[41] to avoid illegal translation fd */
    picSizeInMbs = (pp->wFrameHeightInMbsMinus1 + 1) * (pp->wFrameWidthInMbsMinus1 + 1);
    dirMvOffset  = picSizeInMbs * ((p_hal->pp->chroma_format_idc == 0) ? 256 : 384);
    dirMvOffset += (pp->field_pic_flag && pp->CurrPic.AssociatedFlag) ? (picSizeInMbs * 32) : 0;
    //mpp_log_f("[DEC_OUT]line=%d, cur_slot_idx=%d, outPhyAddr=%d, divMvOffset=%d", __LINE__, pp->CurrPic.Index7Bits, outPhyAddr, (dirMvOffset<<6));
    if (VPUClientGetIOMMUStatus() > 0) {
        FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DIR_MV_BASE, outPhyAddr | (dirMvOffset << 6)));
    } else {
        FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DIR_MV_BASE, outPhyAddr + dirMvOffset));
    }

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_WRITE_MVS_E,     (p_long->nal_ref_idc != 0))); //!< defalut set 1
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DIR_8X8_INFER_E,  pp->direct_8x8_inference_flag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_WEIGHT_PRED_E,    pp->weighted_pred_flag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_WEIGHT_BIPR_IDC,  pp->weighted_bipred_idc));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_REFIDX1_ACTIVE,  (pp->num_ref_idx_l1_active_minus1 + 1)));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_FIELDPIC_FLAG_E, !pp->frame_mbs_only_flag));

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PIC_INTERLACE_E, !pp->frame_mbs_only_flag && (pp->MbaffFrameFlag || pp->field_pic_flag)));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PIC_FIELDMODE_E,  pp->field_pic_flag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PIC_TOPFIELD_E,  !pp->CurrPic.AssociatedFlag)); //!< bottomFieldFlag
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_SEQ_MBAFF_E,      pp->MbaffFrameFlag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_8X8TRANS_FLAG_E,  pp->transform_8x8_mode_flag));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_BLACKWHITE_E,     p_long->profileIdc >= 100 && pp->chroma_format_idc == 0));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_TYPE1_QUANT_E,    pp->scaleing_list_enable_flag));

    if (p_hal->pp->scaleing_list_enable_flag) {
		RK_U32 temp = 0;
		RK_U32 *ptr = NULL;

        ptr = (RK_U32 *)((RK_U8 *)mpp_buffer_get_ptr(p_hal->cabac_buf) + VDPU_CABAC_TAB_SIZE + VDPU_POC_BUF_SIZE);
		for(i = 0; i < 6; i++) {
			for(j = 0; j < 4; j++) {
				temp = (p_hal->qm->bScalingLists4x4[i][4 * j + 0] << 24) |
					   (p_hal->qm->bScalingLists4x4[i][4 * j + 1] << 16) |
					   (p_hal->qm->bScalingLists4x4[i][4 * j + 2] <<  8) |
					   (p_hal->qm->bScalingLists4x4[i][4 * j + 3]);
				*ptr++ = temp;
			}
		}
		for(i = 0; i < 2; i++) {
			for(j = 0; j < 16; j++)	{
				temp = (p_hal->qm->bScalingLists8x8[i][4 * j + 0] << 24) |
				       (p_hal->qm->bScalingLists8x8[i][4 * j + 1] << 16) |
					   (p_hal->qm->bScalingLists8x8[i][4 * j + 2] <<  8) |
					   (p_hal->qm->bScalingLists8x8[i][4 * j + 3]);
				*ptr++ = temp;
			}
		}	
	}

#if 0
	{
		RK_U8 i = 0, j = 0;
		RK_U8 *ptr = (RK_U8 *)mpp_buffer_get_ptr(p_hal->cabac_buf) + VDPU_CABAC_TAB_SIZE + VDPU_POC_BUF_SIZE;
		for(i = 0; i < 6; i++) {
			for(j = 0; j < 16; j++) {
				FPRINT(g_debug_file1, "[i=%2d][j=%2d]=%d, ", i, j, *ptr);
				ptr++;
			}
			FPRINT(g_debug_file1, "\n");
		}
		for(i = 6; i < 8; i++) {
			for(j = 0; j < 64; j++) {
				FPRINT(g_debug_file1, "[i=%2d][j=%2d]=%d, ", i, j, *ptr);
				ptr++;
			}
			FPRINT(g_debug_file1, "\n");
		}
	}
#endif

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_OUT_DIS,     0)); //!< set defalut 0
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_CH_8PIX_ILEAV_E, 0));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_E,           1));

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    return MPP_OK;
__FAILED:
    return ret;
}


