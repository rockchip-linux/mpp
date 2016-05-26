# vim: syntax=cmake
include_directories(.)

# h264 decoder api
set(H264D_API
  ../../inc/h264d_api.h  
  )
set(H264D_COMMON
	../../../common/h264d_log.h
	../../../common/h264d_syntax.h
  )
	
# h264 decoder header	
set(H264D_HDR
	h264d_global.h
	h264d_parse.h
	h264d_slice.h
	h264d_sps.h
	h264d_pps.h
	h264d_scalist.h
	h264d_sei.h
	h264d_dpb.h
	h264d_init.h
	h264d_fill.h
	h264d_rwfile.h
	
	) 
	
# h264 decoder sourse
set(H264D_SRC
	h264d_api.c
	h264d_log.c
	h264d_parse.c
	h264d_slice.c
	h264d_sps.c
	h264d_pps.c
	h264d_scalist.c
	h264d_sei.c
	h264d_dpb.c
	h264d_init.c
	h264d_fill.c
	h264d_rwfile.c
	
	) 

			
add_library(codec_h264d STATIC
				${H264D_API}
				${H264D_COMMON}
				${H264D_HDR} 
				${H264D_SRC} 
			)

target_link_libraries(codec_h264d mpp_base)
set_target_properties(codec_h264d PROPERTIES FOLDER "mpp/codec")

