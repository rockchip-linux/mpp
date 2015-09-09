# vim: syntax=cmake


# h264 decoder api
set(H264D_API
  ../../inc/h264d_api.h  
  )
set(H264D_COMMON
  	../../../common/h264d_rwfile.h
	../../../common/h264d_log.h
	../../../common/h264d_syntax.h
  )
	
# h264 decoder header	
set(H264D_HDR
	h264d_global.h
	h264d_bitread.h
	h264d_parse.h
	) 
	
# h264 decoder sourse
set(H264D_SRC
	h264d_api.c
	h264d_rwfile.c
	h264d_log.c
	h264d_bitread.c
	h264d_parse.c
	) 

			
add_library(codec_h264d STATIC
				${H264D_API}
				${H264D_COMMON}
				${H264D_HDR} 
				${H264D_SRC} 
			)
target_link_libraries(codec_h264d
                      mpp
                      )
set_target_properties(codec_h264d PROPERTIES FOLDER "mpp/codec")

