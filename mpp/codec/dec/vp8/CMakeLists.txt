# vim: syntax=cmake
include_directories(.)

#vp8 decoder api
set(VP8D_API
	../../inc/vp8d_api.h  
  )
  
#vp8 decoder header	
set(VP8D_HDR
	vp8d_parser.h
	vp8d_codec.h
  ) 
	
#vp8 decoder source
set(VP8D_SRC
	vp8d_api.c
        vp8d_parser.c
) 
		
add_library(codec_vp8d STATIC
	  ${VP8D_API}
	  ${VP8D_HDR} 
	  ${VP8D_SRC} 
	  )

target_link_libraries(codec_vp8d  mpp_base)
set_target_properties(codec_vp8d PROPERTIES FOLDER "mpp/codec")

