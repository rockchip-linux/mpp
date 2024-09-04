## 1.0.7 (2024-09-04)
### Feature
- [rc_smt]: Add rc container for smart mode
- [vepu580]: Optimization to improve VMAF
- [vepu580]: Optimize hal processing for smart encoding
- [vepu580]: Add qpmap and rc container interface
- [vepu510]: Add anti-smear regs setup for H.264
- [vepu510]: Add H.264 tuning setup
- [vepu510]: Sync code from enc_tune branch
- [vepu510]: Sync code from enc_tune branch
- [vepu510]: Sync code from enc_tune branch
- [mpp_trie]: Add trie context filling feature
- [mpp_trie]: Add trie tag and shrink feature
- [h264d]: support hdr meta parse
- [h265e]: Support force mark & use ltr
- [vpu_api]: support yuv444sp decode ouput pixel format

### Fix
- [h265d]: fix infochange loss when two sps continuous
- [hal_h264e]: Fix CAVLC encode smartP stream err
- [mpi_enc_test]: Remove redundant code about smart encoding
- [h264e_sps]: fix the default value of max mv length
- [enc_roi]: Fix cu_map init in vepu_54x_roi
- [hal_vp9]: Optimize prob memory usage
- [hal_h265d]: Allow reference missing for GDR
- [osal]: Fix mpp_mem single instance issue
- [hal_vp9d_com]: Fixed memory leak issue
- [hal_h265d]: Avoid risk of segment fault
- [hal_h265d]: fix error slot index marking
- [h265d]: Adjust condition of scan type judgement
- [mpp_hdr]: Fix buffer overflow issue
- [mpp_buffer]: Synchronous log addition point
- [hal_vepu]: fix split regs assignment
- [vepu580]: poll max set to 1 on split out lowdelay mode
- [mpp_common]: fix compile err on F_DUPFD_CLOEXEC not defined
- [h265d]: return error on sps/pps read failure
- [build]: The first toolchains is selected by default
- [265e]:Fix the st refernce frame err in tsvc
- [av1d]: when MetaData found then it is new frame
- [m2vd]: Fix seq_head check error
- [h265e_vepu510]: Fix a memory leak
- [h265d]: auto output frame in dpb when ready
- [m2vd]: Remove ref frame when info changed
- [mpp_meta]: Missing data in the instance
- [mpp_bitread]: Fix negative shift error
- [osal]: fix 128 odd plus 64 bytes alignment
- [h265d_parser]: Fix fmt configuration issue
- [hal_av1d_vdpu383]: modify av1 segid wr/rd base config
- [h265d_parser]: Fix fmt configuration issue
- [hal_av1d_vdpu383]: add segid reg base config

### Docs
- Update 1.0.7 CHANGELOG.md
- [readme]: Add more repo info

### Refactor
- [mpp_cfg]: Refactor MppTrie and string cfg

### Chore
- [mpp_mem]: Add mpp_realloc_size
- [mpp_cfg]: Remove some unused code
- fix compile warning

## 1.0.6 (2024-06-12)
### Feature
- [vdpu383]: refine rcb info setup
- [enc_265]: Support get Largest Code Unit size
- [mpp_dec_cfg]: Add disable dpb check config
- [vdpu383]: support 8K downscale mode

### Fix
- [drm]: Fix permission check issue on GKI kernel
- [hal_h265e]: Amend 510 tid and sync cache
- [hal_h265e]: Fix nalu type avoid stream warning
- [h265e]: Fix vps/sps max temparal layers val
- [hal_jpeg_vdpu1]: fix dec failed on RK3036 problem
- [osal]: rv1109/rv1126 vcodec_type mismatch problem
- [h264e_vepu2]: Adjust inter favor table
- [h264d]: fix drop packets after reset when err stream
- [h265d]: Allow filtering of consecutive start code
- [hal_h264d_vdpu383]: fix spspps update issue
- [mpp]: fix mpp frame leak when async enc
- [enc]: Add use_lt_idx to output packet meta
- [hal_h265e]: fix sse_sum get err
- [mpp_enc_async]: fix mpp packet leak when thread quit
- [enc_roi]: Support ROI cfg under CQP mode
- [hal]: Fix the lib interdependence issue
- [vepu_510]: fix same log type when enc feedback
- [mpp_buffer]: fix dec/inc ref_count in multi threads
- [mpp_enc_async]: fix debreath not work on async flow
- [base]: fix AV1 and AVS2 string info missing problem
- [mpp]: Add encoder input/output pts log
- [hal_vepu580/510]: fix split out err when pass1 frame
- [hal]: Fix target link issue
- [hal_enc]: Fix lib dependency issue
- [hal_h265d_vdpu383]: fix ref_err mark for special poc
- [rc2_test]: fix pkt buffer overflow error
- [enc_utils]: Support read odd resolution image
- [allocator]: fix on invalid DMA heap allocator
- [hal_h265e_vepu580]: fix reg config err for 2pass
- [jpegd_vdpu]: Adjust file dump path
- [mpp_common]: fix 128 odd plus 64 alignment
- [cmake]: fix static build
- [vdpu383]: Update vdpu383 error detection

### Docs
- Update 1.0.6 CHANGELOG.md

### Refactor
- [hal_jpegd]: init devices at hal_jpegd_api
- [dec]: get deocder capability via common routine
- [hal_av1d]: Migrate av1d from vpu to rkdec

### Chore
- [h265d]: Reduce malloc/free frequency of vps
- [mpp_service]: fix typo err
- [hal_h265d]: use INT_MAX for poc distance initiation
- [cmake]: remove duplicate code

## 1.0.5 (2024-04-19)
### Feature
- [vdpu383]: align hor stride to 128 odds + 64 byte
- [vdpu383]: support 2x2 scale down
- [mpp_buffer]: Add MppBuffer attach/detach func
- [mpp_dev]: Add fd attach/detach operation
- [vdpp]: Add libvdpp for hwpq
- [vdpp]: Add capacity check function
- [cmake]: Add building static library
- [vdpp_test]: Add vdpp slt testcase
- [av1d]: Add tile4x4 frame format support
- [mpp_enc_cfg]: Add H.265 tier config
- [jpeg]: Add VPU720 JPEG support
- [enc]: Add config entry for output chroma format
- [vdpu383]: Add vdpu383 av1 module
- [vdpu383]: Add vdpu383 vp9 module
- [vdpu383]: Add vdpu383 avs2 module
- [vdpu383]: Add vdpu383 H.264 module
- [vdpu383]: Add vdpu383 H.265 module
- [vdpu383]: Add vdpu383 common module
- [vdpp]: Add vdpp2 for rk3576
- [vdpp]: Add vdpp module and vdpp_test
- [vepu_510]: Add vepu510 h265e support
- [vepu_510]: Add vepu510 h264e support
- [mpp_frame]: Add tile format flag
- [vepu_510] add vepu 510 common for H264 & h265
- [mpp_soc]: support rk3576 soc

### Fix
- [avs2d_vdpu383]: Optimise dec result
- [vdpu383]: Fix compiler warning
- [vdpp]: Fix dmsr reg size imcompat error
- [vdpu383]: hor stride alignment fix for vdpu383
- [h265d_ref]: fix set fbc output fmt not effect issue
- [vdpu383]: Fix memory out of bounds issue
- [h264d_vdpu383]: Fix global parameter config issue
- [avs2_parse]: add colorspace config to mpp_frame
- [hal_h264e]:fix crash after init vepu buffer failure
- [vpu_api]: Fix frame format and eos cfg
- [av1d_vdpu383]: fix fbc hor_stride error
- [av1d_parser]: fix fmt error for 10bit HDR source
- [avs2d]: fix stuck when seq_end shows at pkt tail
- [av1d_vdpu]: Fix forced 8bit output failure issue
- [enc_async]: Invalidate cache for output buffer
- [hal_av1d_vdpu383]: memleak for cdf_bufs
- [av1d_vdpu383]: fix rcb buffer size
- [vp9d_vdpu383]: Fix segid config issue
- [vepu510]: Add split low delay output mode support
- [avs2d_vdpu383]: Fix declaring shadow local variables issue
- [av1]: Fix global config issue
- [hal_av1d]: Delte cdf unused value
- [av1]: Fix av1 display abnormality issue
- [avs2d]: Remove a unnecessary log
- [vepu510]: Adjust regs assignment
- [hal_jpegd]: Add stream buffer flush
- [265e_api]: Support cons_intra_pred_flag cfg
- [mpp_enc]: Add device attach/detach on enc flow
- [mpp_dec]: Add device attach/detach on dec flow
- [vdpp]: Add error detection
- [hal_265e_510]: modify srgn_max & rime_lvl val
- [vdpu383]: spspps data not need copy all range ppsid
- [vpu_api_legacy]: fix frame CodingType err
- [av1]: Fix 10bit source display issue
- [mpp_enc]: Expand the hdr_buf size
- [av1]: Fix delta_q value read issue
- [vdpu383]: Enable error detection
- [ext_dma]: fix mmap permission error
- [jpege_vpu720]: sync cache before return task
- [mpp_buffer]: fix buffer type assigning
- [vepu510]: Configure reg of Subjective param
- [vepu510]: Checkout and optimize 510 reg.h
- [mpp_dec]: Optimize HDR meta process
- [av1d]: Fix scanlist calc issue
- [h265e]: fix the profile tier cfg
- [av1d]: Fix av1d ref stride error
- [hal_h265e_vepu510]: Add cudecis reg cfg
- [av1d]: Only rk3588 support 10bit translate to 8bit
- [vp9d]: Fix vp9 hor stride issue
- [rc]: Add i quality delta cfg on fixqp mode
- [hal_h265d]: Fix filter col rcb buffer size calc
- [av1d]: Fix compiler warning
- [h264d]: Fix error mvc stream crash issue
- [hal_h264e]: Fix qp err when fixqp mode
- [h264d]: Fix H.264 error chroma_format_idc
- [vdpu383]: Fix av1 rkfbc output error
- [av1d]: Fix compatibility issues
- [hal_h264d_vdpu383]: Reduce mpp_put_bits calls
- Fix clerical error
- [avs2d]: Fix get ref_frm slot idx error
- [vdpu383]: Fix av1 global params bit pos issue
- [vdpp]: fix sharp config error
- [hal_av1d]: fix av1 dec err for rk3588
- [vdpu383]: Fix av1 global params issue
- [vepu510]: Fix camera record stuck issue
- [utils]: fix read and write some YUV format
- [mpp_bitput]: fix put bits overflow
- [mpp_service]: fix rcb info env config
- [vepu510]: Fix compile warning
- [hal_vp9d]: fix colmv size calculator err
- [avsd]: Fix the ref_frm slot idx erro in fast mode.

### Docs
- Update 1.0.5 CHANGELOG.md
- [mpp_frame]: Add MppFrameFormat description

### Refactor
- [hal_av2sd]: refactor hal_api assign flow
- [hal_h264d]: refactor hal_api assign flow
- Using soc_type for compare instead of soc_name

### Chore
- [hal_h264e]: clean some unused code

## 1.0.4 (2024-02-07)
### Feature
- [vpu_api_legacy]: Support RGB24 setup
- [avsd]: keep codec type if not avs+
- [mpi_enc_test]: add YUV400 fmt support
- [mpp_enc]: Add YUV400 support for vepu580/540

### Fix
- [h265e]: fix hw stream size check error
- [hal_vdpu]: unify colmv buffer size calculation
- [vproc]: Fix deadlock in vproc thread
- [h265e]: disable tmvp by default
- [h265e]: Amend temporal_id to stream
- [mpp_dump]: add YUV420SP_10BIT format dump
- [hal_h265d]: Fix register length for rk3328/rk3328H
- [hal_avsd]: Fix crash on no buffer decoded
- [mpp_enc]: allow vp8 to cfg force idr frame
- [m2vd]: fix unindentical of input and output pts list
- [h265e_vepu580]: fix SIGSEGV when reencoding
- [mpp_dmabuf]: fix align cache line size calculate err
- [h265e_vepu580]: flush cache for the first tile
- [dmabuf]: Disable dmabuf partial sync function
- [iep_test]: use internal buffer group
- [common]: Add mpp_dup function
- [h265e]: Adapter RK3528 when encoding P frame skip
- [h265e]: fix missing end_of_slice_segment_flag problem
- [hal_av1d_vdpu]: change rkv_hor_align to 16 align
- [av1d_parser]: set color info per frame
- [jpegd]: add sof marker check when parser done

### Docs
- Update 1.0.4 CHANGELOG.md

### Chore
- [script]: add rebuild and clean for build
- [mpp_enc_roi_utils]: change file format dos to unix

## 1.0.3 (2023-12-08)
### Feature
- [dec_test]: Add buffer mode option
- [mpp_dmabuf]: Add dmabuf sync operation
- [jpege]: Allow rk3588 jpege 4 tasks async
- [rc_v2]: Support flex fps rate control

### Fix
- [av1d_api]: fix loss last frame when empty eos
- [h265e_dpb]: do not check frm status when pass1
- [hal_bufs]: clear buffer when hal_bufs get failed
- [dma-buf]: Add dma-buf.h for old ndk compiling
- [enc]: Fix sw enc path segment_info issue
- [cmake]: Remove HAVE_DRM option
- [m2vd]: update frame period on frame rate code change
- [test]: Fix mpi_enc_mt_test error
- [dma_heap]: add dma heap uncached node checking
- [mpp_mem]: Fix MEM_ALIGNED macro error
- [mpeg4_api]: fix drop frame when two stream switch
- [script]: fix shift clear input parameter error
- [hal_h265e_vepu541]: fix roi buffer variables incorrect use

### Docs
- Update 1.0.3 CHANGELOG.md

### Refactor
- [allocator]: Refactor allocator flow

### Chore
- [vp8d]: optimize vp8d debug
- [mpp_enc]: Encoder changes to cacheable buffer
- [mpp_dec]: Decoder changes to cacheable buffer
- [mpp_dmabuf]: Add dmabuf ioctl unit test

## 1.0.2 (2023-11-01)
### Feature
- [mpp_lock]: Add spinlock timing statistic
- [mpp_thread]: Add simple thread
- add more enc info to meta

### Fix
- [vepu540c]: fix h265 config
- [h264d]: Optimize sps check error
- [utils]: adjust format range constraint
- [h264d]: fix mpp split eos process err
- [h264d]: add errinfo for 4:4:4 lossless mode
- [h264d]: fix eos not updated err
- [camera_source]: Fix memory double-free issue
- [mpp_dec]:fix mpp_destroy crash
- [mpp_enc]: Fix async multi-thread case error
- [jpeg_dec]: Add parse & gen_reg err check for jpeg dec
- [h265e_vepu580]: fix tile mode cfg
- [vp9d]: Fix AFBC to non-FBC mode switch issue
- [h264e_dpb]: fix modification_of_pic_nums_idc error issue
- [allocator]: dma_heap allocator has the highest priority
- [camera_source]: enumerate device and format
- [utils]: fix hor_stride 24 aligned error

### Docs
- Update 1.0.2 CHANGELOG.md
- Add mpp developer guide markdown

### Chore
- [scipt]: Update changelog.sh

## 1.0.1 (2023-09-28)
### Feature
- [venc]: Modify fqp init value to invalid.
- [vepu580]: Add frm min/max qp and scene_mode cmd param
- [venc]: Add qbias for rkvenc encoder
- Support fbc mode change on info change stage
- [hal_vepu5xx]: Expand color transform for 540c
- Add rk3528a support

### Fix
- [mpp_enc_impl]: fix some error values without return an error
- [av1d_cbs]: fix read 32bit data err
- [Venc]: Fix jpeg and vpx fqp param error.
- [h265e_vepu580]: dual cores concurrency problem
- [hal_h264e_vepu]: terminate task if not support
- [vdpu_34x]: disable cabac err check for rk3588
- [enc]: fix duplicate idr frame
- [h264e_amend]: fix slice read overread issue
- [hal_jpegd]: add pp feature check
- [enc]: fix duplicate sps/pps information
- [h264e_slice]: fix pic_order_cnt_lsb wrap issue
- [hal_h264e_vepu540c]: fix reg configuration
- [hal_h264e_vepu540c]: Amend slice header
- [h264d]: fix crash on check reflist
- [hal_vp9d]: not support fast mode for rk3588
- [h264d]: fix frame output order when dpb full
- [mpp_frame]: setup color default UNSPECIFIED instead 0
- [hal_h264d]: adjust position of env_get
- [h264e_slice]: fix pic_order_cnt_lsb wrap issue
- [hal_avs2d]: fix some issue
- fix redundant prefix NALU amended problem
- [hal_jpegd]: fix rgb out_fmt mismatch error
- [utils]: fix convert format error
- [h265e]: check input profile_idc
- [hal_h264e_vepu580]: fix SEGV on GDR setting
- [h264d]: fix TSVC decode assert error.
- [hal_vepu580]: fix comiple issue
- [h264d]: fix MVC DPB allocation
- [h264d]: fix SEI packet parsing
- [hal_vp8e]: fix entropy init
- [mpp_soc]: fix rk356x vepu2 capability

### Docs
- Add 1.0.1 CHANGELOG.md
- update readme.txt

### Refactor
- move same tables to common module

## 1.0.0 (2023-07-26)
### Docs
- Add 1.0.0 CHANGELOG.md
