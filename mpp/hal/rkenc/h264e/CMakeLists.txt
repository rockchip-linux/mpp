# vim: syntax=cmake
include_directories(.)

# hal h264 encoder api
set(HAL_H264E_API
	../../inc/hal_h264e_api.h
    ../../../common/h264e_syntax.h
    )

    
# hal h264 header   
set(HAL_H264E_HDR
	hal_h264e.h
	hal_h264e_vpu.h
	hal_h264e_rkv.h	
    ) 
    
# hal h264 encoder sourse
set(HAL_H264E_SRC
	hal_h264e_api.c
	hal_h264e_vpu.c
	hal_h264e_rkv.c
    ) 
    
add_library(hal_h264e STATIC
            ${HAL_H264E_API}
            ${HAL_H264E_HDR} 
            ${HAL_H264E_SRC} 
            )
if(RKPLATFORM)
    target_link_libraries(hal_h264e worker_vpu mpp_base)
else()
    target_link_libraries(hal_h264e mpp_base)
endif()    

set_target_properties(hal_h264e PROPERTIES FOLDER "mpp/hal")   
     
add_subdirectory(test)
