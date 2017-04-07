MPP (Media Process Platform) design (2016.10.12)
================================================================================

The mpp is a middleware library for Rockchip SoC's cross platform media process.
The main purpose of mpp is to provide very high performance, high flexibility
and expansibility on multimedia (mainly video and image) process.

The design target of mpp is to connect different Rockchip hardware kernel driver
and different userspace application.

Rockchip has two sets of hardware kernel driver.

The first one is vcodec_service/vpu_service/mpp_service which is a high
performance stateless frame base hardware kernel driver. This driver supports
all available codecs that hardware can provide. This driver is used on Android/
Linux.

The second one is v4l2 driver which is developed for ChromeOS. It currently
supports H.264/H.265/vp8/vp9. This driver is used on ChomeOS/Linux.

Mpp plans to support serval userspace applications including OpenMax, FFmpeg,
gstreamer, libva.


Feature explaination
================================================================================

1. Cross Platform
The target OS platform including Android, Linux, ChromeOS and windows.  Mpp uses
cmake to compile on different platform.

2. High Performance
Mpp supports sync / async interface to reduce the time blocked in interface. And
mpp internally make hardware and software run parallelly. When hardware is
running sofware will prepare next hardware task at the same time.

3. High Flexibility
mpi (Media Process Interface) is easy to extend by different control function.
The input/output element packet/frame/buffer is easy to extend different
components.


System Diagram
================================================================================

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
            |   |  control  | |  reg_gen  |    |        |   |
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

Mpp is composed of four main sub-modules:

OSAL (Operation System Abstraction Layer)
This module shutters the differences between different operation systems and
provide basic components including memory, time, thread, log and hardware memory
allocator.

MPI (Media Process Interface) / MPP
This module is on charge of interaction with external user. Mpi layer has two
ways for user. The simple way - User can use put/get packet/frame function set.
The advanced way - User has to config MppTask and use dequeue/enqueue funciton
set to communicate with mpp. MppTask can carry different meta data and complete
complex work.

Codec (encoder / decoder)
This module implements the high efficiency internal work flow. The codec module
provides a general call flow for different video format. Software process will
be separated from hardware specified process. The software will communicate with
hardware with a common task interface which combines the buffer information and
codec specified infomation.

Parser/Controller and hal (Hardware Abstraction Layer)
This layer provides the implement function call of different video format and
different hardware. For decoder parser provide the video stream parse function
and output format related syntax structure to hal. The hal will translate the
syntax structure to register set on different hardware. Current hal supports
vcodec_service kernel driver and plan to support v4l2 driver later.


