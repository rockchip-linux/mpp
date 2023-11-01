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
