# vim: syntax=cmake

# h264 decoder api
set(H264D_API
  ../../inc/h264d_api.h
  
  )

	
# h264 decoder header	
set(H264D_HDR

	) 
	
# h264 decoder sourse
set(H264D_SRC
	h264d_api.c
	) 

			
add_library(codec_h264d STATIC
				${H264D_API}
				${H264D_HDR} 
				${H264D_SRC} 
			)
				
set_target_properties(codec_h264d PROPERTIES FOLDER "mpp/codec")

