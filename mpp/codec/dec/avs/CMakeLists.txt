# vim: syntax=cmake
include_directories(.)

# avs decoder api
set(AVSD_API

  )
set(AVSD_COMMON

  )
	
# avs decoder header	
set(AVSD_HDR
    avsd_parse.h
	) 
	
# avs decoder sourse
set(AVSD_SRC
	avsd_api.c
	avsd_parse.c
	) 

add_library(codec_avsd STATIC
			${AVSD_API}
			${AVSD_COMMON}
			${AVSD_HDR} 
			${AVSD_SRC} 
		)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/avs_impl/")
add_subdirectory(avs_impl)
target_link_libraries(codec_avsd codec_avsd_impl)
else()
set(LIBAVS "${CMAKE_CURRENT_SOURCE_DIR}/libavs.a")
add_library(codec_avsd_impl STATIC IMPORTED)
set_target_properties(codec_avsd_impl PROPERTIES IMPORTED_LOCATION ${LIBAVS})
target_link_libraries(codec_avsd codec_avsd_impl)
endif()		
target_link_libraries(codec_avsd mpp)
set_target_properties(codec_avsd PROPERTIES FOLDER "mpp/codec")	




