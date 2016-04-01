# vim: syntax=cmake
include_directories(.)

# avs decoder api
set(AVSD_API

  )
set(AVSD_COMMON

  )
	
# avs decoder header	
set(AVSD_HDR

	) 
	
# avs decoder sourse
set(AVSD_SRC
	avsd_api.c
	) 

add_library(codec_avsd STATIC
			${AVSD_API}
			${AVSD_COMMON}
			${AVSD_HDR} 
			${AVSD_SRC} 
		)
if(EXISTS codec_avsd_impl)
    target_link_libraries(codec_avsd codec_avsd codec_avsd_impl)
else()
    target_link_libraries(codec_avsd mpp)
endif()

set_target_properties(codec_avsd PROPERTIES FOLDER "mpp/codec")	

