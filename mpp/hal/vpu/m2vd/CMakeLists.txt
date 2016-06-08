# vim: syntax=cmake
include_directories(.)

# hal m2v decoder api
set(HAL_M2VD_API
	../../inc/hal_m2vd_api.h
    ../../../common/m2vd_syntax.h
    )

    
# hal m2v header   
set(HAL_M2VD_HDR
	hal_m2vd_reg.h
    ) 
    
# hal m2v decoder sourse
set(HAL_M2VD_SRC
	hal_m2vd_api.c
	hal_m2vd_reg.c
    ) 
    
add_library(hal_m2vd STATIC
            ${HAL_M2VD_API}
            ${HAL_M2VD_HDR} 
            ${HAL_M2VD_SRC} 
            )
if(ANDROID)
    target_link_libraries(hal_m2vd vpu osal)
else()
    target_link_libraries(hal_m2vd osal)
endif()    
target_link_libraries(hal_m2vd
                      mpp_base
					  )
set_target_properties(hal_m2vd PROPERTIES FOLDER "mpp/hal")        

