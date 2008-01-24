# vim: syntax=cmake

# hal h264 decoder api
set(HAL_H264D_API
  ../../inc/hal_h264d_api.h
  
  )

	
# hal h264 header	
set(HAL_H264D_HDR

	) 
	
# hal h264 decoder sourse
set(HAL_H264D_SRC
	hal_h264d_api.c
	
	) 
	
add_library(hal_h264d STATIC
				${HAL_H264D_API}
				${HAL_H264D_HDR} 
				${HAL_H264D_SRC} 
			)
					
set_target_properties(hal_h264d PROPERTIES FOLDER "mpp/hal")		
