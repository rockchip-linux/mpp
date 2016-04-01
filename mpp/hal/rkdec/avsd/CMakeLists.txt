# vim: syntax=cmake

# hal avs decoder api
set(HAL_AVSD_API

    )

    
# hal avs header   
set(HAL_AVSD_HDR

    ) 
    
# hal avs decoder sourse
set(HAL_AVSD_SRC

    hal_avsd_api.c	

    ) 
    
add_library(hal_avsd STATIC
            ${HAL_AVSD_API}
            ${HAL_AVSD_HDR} 
            ${HAL_AVSD_SRC} 
            )
target_link_libraries(hal_avsd
                      mpp
                      )
					  
set_target_properties(hal_avsd PROPERTIES FOLDER "mpp/hal")        

