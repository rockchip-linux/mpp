Media Process Platform (MPP) module directory description:

MPP    : Media Process Platform
MPI    : Media Process Interface
HAL    : Hardware Abstract Layer
OSAL   : Operation System Abstract Layer

Rules:
1. header file arrange rule
a. inc directory in each module folder is for external module usage.
b. module internal header file should be put along with the implement file.
c. header file should not contain any relative path or absolute path, all
   include path should be keep in Makefile.
2. compiling system rule
a. for cross platform compiling use cmake as the compiling management system.
b. use cmake out-of-source build, final binary and library will be install to
   out/ directory.
3. header file include order
a. MODULE_TAG
b. system header
c. osal header
d. module header

NOTE:
1. Windows support is NOT maintained any more.
2. Mpp support all rockchip chipset now including:
   RK29XX/RK30XX/RK31XX
   RK3288/RK3368/RK3399
   RK3228/RK3229/RK3228H/RK3328
   RV1108/RV1107
3. Mpp support all format hardware can support except VC1.
4. you can get demo about mpp applied to linux and android.
     Liunx : https://github.com/WainDing/mpp_linux_cpp
     Android : https://github.com/c-xh/MediaCodecDecodeMulti_h264

More document can be found at http://opensource.rock-chips.com/wiki_Mpp

----                             top
   |
   |----- build                  CMake out-of-source build directory
   |  |
   |  |----- cmake               cmake script directory
   |  |
   |  |----- android             android build directory
   |  |
   |  |----- linux               linux build directory
   |  |
   |  |----- vc10-x86_64         visual studio 2010 on x86_64 build directory
   |  |
   |  |----- vc12-x86_64         visual studio 2013 on x86_64 build directory
   |
   |----- doc                    design documents of mpp
   |
   |----- inc                    header file for external usage, including
   |                             platform header and mpi header
   |
   |----- mpp                    Media Process Platform : mpi function private
   |  |                          implement and mpp infrastructure (vpu_api
   |  |                          private layer)
   |  |
   |  |----- base                base components including MppBuffer, MppFrame,
   |  |                          MppPacket, MppTask, MppMeta, etc.
   |  |
   |  |----- common              video codec protocol syntax interface for both
   |  |                          codec parser and hal
   |  |
   |  |----- codec               all video codec parser, convert stream to
   |  |  |                       protocol structure
   |  |  |
   |  |  |----- inc              header files provided by codec module for
   |  |  |                       external usage
   |  |  |
   |  |  |----- dec
   |  |  |  |
   |  |  |  |----- dummy         decoder parser work flow sample
   |  |  |  |
   |  |  |  |----- h263
   |  |  |  |
   |  |  |  |----- h264
   |  |  |  |
   |  |  |  |----- h265
   |  |  |  |
   |  |  |  |----- m2v           mpeg2 parser
   |  |  |  |
   |  |  |  |----- mpg4          mpeg4 parser
   |  |  |  |
   |  |  |  |----- vp8
   |  |  |  |
   |  |  |  |----- vp9
   |  |  |  |
   |  |  |  |----- jpeg
   |  |  |
   |  |  |----- enc
   |  |     |
   |  |     |----- dummy         encoder controllor work flow sample
   |  |     |
   |  |     |----- h264
   |  |     |
   |  |     |----- h265
   |  |     |
   |  |     |----- jpeg
   |  |
   |  |----- hal                 Hardware Abstract Layer (HAL): modules used in mpi
   |  |  |
   |  |  |----- inc              header files provided by hal for external usage
   |  |  |
   |  |  |----- iep              iep user library
   |  |  |
   |  |  |----- pp               post-processor user library
   |  |  |
   |  |  |----- rga              rga user library
   |  |  |
   |  |  |----- deinter          deinterlace function module including pp/iep/rga
   |  |  |
   |  |  |----- rkdec            rockchip hardware decoder register generation
   |  |  |  |
   |  |  |  |----- h264d         generate register file from H.264 syntax info
   |  |  |  |
   |  |  |  |----- h265d         generate register file from H.265 syntax info
   |  |  |  |
   |  |  |  |----- vp9d          generate register file from vp9 syntax info
   |  |  |
   |  |  |----- vpu              vpu register generation library
   |  |     |
   |  |     |----- h263d         generate register file from H.263 syntax info
   |  |     |
   |  |     |----- h264d         generate register file from H.264 syntax info
   |  |     |
   |  |     |----- h265d         generate register file from H.265 syntax info
   |  |     |
   |  |     |----- jpegd         generate register file from jpeg syntax info
   |  |     |
   |  |     |----- jpege         generate register file from jpeg syntax info
   |  |     |
   |  |     |----- m2vd          generate register file from mpeg2 syntax info
   |  |     |
   |  |     |----- mpg4d         generate register file from mpeg4 syntax info
   |  |     |
   |  |     |----- vp8d          generate register file from vp8 syntax info
   |  |
   |  |----- legacy              generate new libvpu to include old vpuapi path
   |  |                          and new mpp path
   |  |
   |  |----- test                mpp internal video protocol unit test and demo
   |
   |----- test                   mpp buffer/packet component unit test and
   |                             mpp/mpi/vpu_api demo
   |
   |----- out                    final release binary output directory
   |  |
   |  |----- bin                 executable binary file output directory
   |  |
   |  |----- inc                 header file output directory
   |  |
   |  |----- lib                 library file output directory
   |
   |----- osal                   Operation System Abstract Layer: abstract layer
   |  |                          for different operation system
   |  |
   |  |----- allocator           supported allocator including Android ion and
   |  |                          Linux drm
   |  |
   |  |----- android             google's android
   |  |
   |  |----- inc                 osal header file for mpp modules
   |  |
   |  |----- linux               mainline linux kernel
   |  |
   |  |----- windows             microsoft's windows
   |  |
   |  |----- test                OASL unit test
   |
   |----- tools                  coding style format tools
   |
   |----- utils                  small util functions


Here is the mpp implement overall framework:

                +---------------------------------------+
                |                                       |
                | ffmpeg / OpenMax / gstreamer / libva  |
                |                                       |
                +---------------------------------------+

            +-------------------- MPP ----------------------+
            |                                               |
            |   +-------------------------+    +--------+   |
            |   |                         |    |        |   |
            |   |        MPI / MPP        |    |        |   |
            |   |   buffer queue manage   |    |        |   |
            |   |                         |    |        |   |
            |   +-------------------------+    |        |   |
            |                                  |        |   |
            |   +-------------------------+    |        |   |
            |   |                         |    |        |   |
            |   |          codec          |    |  OSAL  |   |
            |   |    decoder / encoder    |    |        |   |
            |   |                         |    |        |   |
            |   +-------------------------+    |        |   |
            |                                  |        |   |
            |   +-----------+ +-----------+    |        |   |
            |   |           | |           |    |        |   |
            |   |  parser   | |    HAL    |    |        |   |
            |   |  recoder  | |  reg_gen  |    |        |   |
            |   |           | |           |    |        |   |
            |   +-----------+ +-----------+    +--------|   |
            |                                               |
            +-------------------- MPP ----------------------+

                +---------------------------------------+
                |                                       |
                |                kernel                 |
                |       RK vcodec_service / v4l2        |
                |                                       |
                +---------------------------------------+



Here is the Media Process Interface hierarchical structure
MpiPacket and MpiFrame is the stream I/O data structure.
And MpiBuffer encapsulates different buffer implement like Linux's dma-buf and
Android's ion.
This part is learned from ffmpeg.

                +-------------------+
                |                   |
                |        MPI        |
                |                   |
                +---------+---------+
                          |
                          |
                          v
                +---------+---------+
                |                   |
            +---+        ctx        +---+
            |   |                   |   |
            |   +-------------------+   |
            |                           |
            v                           v
    +-------+-------+           +-------+-------+
    |               |           |               |
    |     packet    |           |     frame     |
    |               |           |               |
    +---------------+           +-------+-------+
            |                           |
            |                           |
            |                           |
            |     +---------------+     |
            |     |               |     |
            +---->+     buffer    +<----+
                  |               |
                  +---------------+



Take H.264 deocder for example. Video stream will first queued by MPI/MPP layer,
MPP will send the stream to codec layer, codec layer parses the stream header
and generates a protocol standard output. This output will be send to HAL to
generate register file set and communicate with hardware. Hardware will complete
the task and resend information back. MPP notify codec by hardware result, codec
output decoded frame by display order.

MPI                MPP              decoder             parser              HAL

 +                  +                  +                  +                  +
 |                  |                  |                  |                  |
 |   open context   |                  |                  |                  |
 +----------------> |                  |                  |                  |
 |                  |                  |                  |                  |
 |       init       |                  |                  |                  |
 +----------------> |                  |                  |                  |
 |                  |                  |                  |                  |
 |                  |       init       |                  |                  |
 |                  +----------------> |                  |                  |
 |                  |                  |                  |                  |
 |                  |                  |       init       |                  |
 |                  |                  +----------------> |                  |
 |                  |                  |                  |                  |
 |                  |                  |                  |       open       |
 |                  |                  +-----------------------------------> |
 |                  |                  |                  |                  |
 |      decode      |                  |                  |                  |
 +----------------> |                  |                  |                  |
 |                  |                  |                  |                  |
 |                  |   send_stream    |                  |                  |
 |                  +----------------> |                  |                  |
 |                  |                  |                  |                  |
 |                  |                  |   parse_stream   |                  |
 |                  |                  +----------------> |                  |
 |                  |                  |                  |                  |
 |                  |                  |                  |  reg generation  |
 |                  |                  +-----------------------------------> |
 |                  |                  |                  |                  |
 |                  |                  |                  |    send_regs     |
 |                  |                  +-----------------------------------> |
 |                  |                  |                  |                  |
 |                  |                  |                  |    wait_regs     |
 |                  |                  +-----------------------------------> |
 |                  |                  |                  |                  |
 |                  |                  |  notify_hw_end   |                  |
 |                  |                  +----------------> |                  |
 |                  |                  |                  |                  |
 |                  |   get_picture    |                  |                  |
 |                  +----------------> |                  |                  |
 |                  |                  |                  |                  |
 |                  |                  |   get_picture    |                  |
 |                  |                  +----------------> |                  |
 |                  |                  |                  |                  |
 |      flush       |                  |                  |                  |
 +----------------> |                  |                  |                  |
 |                  |                  |                  |                  |
 |                  |      flush       |                  |                  |
 |                  +----------------> |                  |                  |
 |                  |                  |                  |                  |
 |                  |                  |      reset       |                  |
 |                  |                  +----------------> |                  |
 |                  |                  |                  |                  |
 |      close       |                  |                  |                  |
 +----------------> |                  |                  |                  |
 |                  |                  |                  |                  |
 |                  |      close       |                  |                  |
 |                  +----------------> |                  |                  |
 |                  |                  |                  |                  |
 |                  |                  |      close       |                  |
 |                  |                  +----------------> |                  |
 |                  |                  |                  |                  |
 |                  |                  |                  |      close       |
 |                  |                  +-----------------------------------> |
 +                  +                  +                  +                  +

There are three memory usage modes that decoder can support:

Mode 1: Pure internal mode
In the mode user will NOT call MPP_DEC_SET_EXT_BUF_GROUP control to decoder.
Only call MPP_DEC_SET_INFO_CHANGE_READY to let decoder go on. Then decoder will
use create buffer internally and user need to release each frame they get.

Advantage:
Easy to use and get a demo quickly
Disadvantage:
1. The buffer from decoder may not be return before decoder is close.
   So memroy leak or crash may happen.
2. The decoder memory usage can not be control. Decoder is on a free-to-run
   status and consume all memory it can get.
3. Difficult to implement zero-copy display path.

Mode 2: Half internal mode
This is the mode current mpi_dec_test code using. User need to create
MppBufferGroup according to the returned info change MppFrame.
User can use mpp_buffer_group_limit_config to limit decoder memory usage.

Advantage:
1. Easy to use
2. User can release MppBufferGroup after decoder is closed.
   So memory can stay longer safely.
3. Can limit the memory usage by mpp_buffer_group_limit_config
Disadvantage:
1. The buffer limitation is still not accurate. Memory usage is 100% fixed.
2. Also difficult to implement zero-copy display path.

Mode 3: Pure external mode
In this mode use need to create empty MppBufferGroup and import memory from
external allocator by file handle.
On Android surfaceflinger will create buffer. Then mediaserver get file handle
from surfaceflinger and commit to decoder's MppBufferGroup.

Advantage:
1. Most efficient way for zero-copy display
Disadvantage:
1. Difficult to learn and use.
2. Player work flow may limit this usage.
3. May need external parser to get the correct buffer size for the external
   allocator.

The required buffer size caculation:
hor_stride * ver_stride * 3 / 2 for pixel data
hor_stride * ver_stride / 2 for extra info
Total hor_stride * ver_stride * 2 will be enough.

For H.264/H.265 20+ buffers will be enough.
For other codec 10 buffers will be enough.
