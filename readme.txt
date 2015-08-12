Media Process Platform (MPP) module directory description:

MPP    : Media Process Platform
MPI    : Media Process Interface
HAL    : Hardware Abstract Layer
OSAL   : Operation System Abstract Layer

Rules:
1. header file arrange rule
a. inc directory in each module folder is for external module usage.
b. module internal header file should be put along with the implement file.
c. header file should not contain any relative path or absolute path, all include path should be keep in Makefile.
2. compiling system rule
a. for cross platform compiling use cmake as the compiling management system.
b. use cmake out-of-source build, final binary and library will be install to out/ directory.
3. header file include order
a. MODULE_TAG
b. system header
c. osal header
d. module header

NOTE:
1. when run on window pthreadVC2.dll needed to be copied to the system directory.

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
   |  |----- vc12-x86_64         visual studio 2013 on x86_64 build directory
   |                             
   |----- inc                    header file for external usage, including platform header and mpi header
   |                             
   |----- mpi                    Media Process Interface: the api function implement in public (vpu_api layer)
   |                             
   |----- mpp                    Media Process Platform : mpi function private implement and mpp infrastructure (vpu_api private layer)
   |  |                          
   |  |----- codec               all video codec parser, convert stream to protocol structure
   |  |  |                       
   |  |  |----- inc              header files provided by codec module for external usage
   |  |  |
   |  |  |----- dec
   |  |  |  |
   |  |  |  |----- h264
   |  |  |  |
   |  |  |  |----- h265
   |  |  |  |
   |  |  |  |----- vp9
   |  |  |  |
   |  |  |  |----- jpeg
   |  |  |
   |  |  |----- enc
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
   |  |  |----- rkdec            rockchip hardware decoder register generation library
   |  |  |  |                    
   |  |  |  |----- h264d         generate register file from H.264 structure created by codec parser
   |  |  |  |                    
   |  |  |  |----- h265d         generate register file from H.265 structure created by codec parser
   |  |  |  |                    
   |  |  |  |----- vp9d          generate register file from vp9 structure created by codec parser
   |  |  |                       
   |  |  |----- vpu              vpu register generation library
   |  |     |                    
   |  |     |----- h264d         generate register file from H.264 structure created by codec parser
   |  |     |                    
   |  |     |----- h265d         generate register file from H.265 structure created by codec parser
   |  |                          
   |  |----- legacy              legacy vpu_api interface
   |  |                          
   |  |----- syntax              syntax interface for different video codec protocol
   |                             
   |----- test                   mpi/mpp unit test files and mpi demo files
   |                             
   |----- out                    final release binary output directory
   |  |                          
   |  |----- bin                 executable binary file output directory
   |  |                          
   |  |----- inc                 header file output directory
   |  |                          
   |  |----- lib                 library file output directory
   |                             
   |----- osal                   Operation System Abstract Layer: abstract layer for different operation system
      |                          
      |----- mem     	         mpi memory subsystem for hardware
      |                          
      |----- android             google's android
      |                          
      |----- linux               mainline linux kernel
      |                          
      |----- window              microsoft's window
      |                          
      |----- test                OASL unit test
    

Here is the mpp implement overall framework:

	+-------------------------+    +--------+
	|                         |    |        |
	|        MPI / MPP        |    |        |
	|   buffer queue manage   |    |        |
	|                         |    |        |
	+-------------------------+    |        |
								   |        |
	+-------------------------+    |        |
	|                         |    |        |
	|          codec          |    |  OSAL  |
	|    decoder / encoder    |    |        |
	|                         |    |        |
	+-------------------------+    |        |
								   |        |
	+-----------+ +-----------+    |        |
	|           | |           |    |        |
	|  parser   | |    HAL    |    |        |
	|  recoder  | |  reg_gen  |    |        |
	|           | |           |    |        |
	+-----------+ +-----------+    +--------+


Take H.264 deocder for example. Video stream will first queued by MPI/MPP layer, MPP will send the stream to codec layer,
codec layer parses the stream header and generates a protocol standard output. This output will be send to HAL to generate
register file set and communicate with hardware. Hardware will complete the task and resend information back. MPP notify
codec by hardware result, codec output decoded frame by display order.

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

