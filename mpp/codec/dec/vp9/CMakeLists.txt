# vim: syntax=cmake
include_directories(.)

# vp9 decoder api
set(VP9_D_API
  ../../inc/vp9d_api.h  
  )

	
# h264 decoder header	
set(VP9D_HDR


	
	
	) 
	
# h264 decoder sourse
set(VP9D_SRC
    vp9d_api.c
    vp9d_parser.c
    vpx_rac.c
    vp9d_parser2_syntax.c		
	) 

			
add_library(codec_vp9d STATIC
				${VP9D_API}
				${VP9D_HDR} 
				${VP9D_SRC} 
			)

target_link_libraries(codec_vp9d mpp_base)
set_target_properties(codec_vp9d PROPERTIES FOLDER "mpp/codec")

