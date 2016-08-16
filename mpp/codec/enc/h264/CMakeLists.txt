# vim: syntax=cmake
include_directories(.)
include_directories(include)

# h264 encoder api
set(H264E_API
#  ../../inc/h264d_api.h  
  )
set(H264E_COMMON
#	../../../common/h264d_log.h
#	../../../common/h264d_syntax.h
  )
	
# h264 encoder header	
set(H264E_HDR
  include/H264CodeFrame.h
  include/H264Init.h
  include/H264Instance.h
  include/H264Mad.h
  include/H264NalUnit.h
  include/H264PictureParameterSet.h
  include/H264PutBits.h
  include/H264RateControl.h
  include/H264Sei.h
  include/H264SequenceParameterSet.h
  include/H264Slice.h
  include/encasiccontroller.h
  include/enccommon.h
  include/encpreprocess.h
  include/h264encapi.h
  include/h264e_codec.h
  include/h264e_utils.h
  ) 
	
# h264 encoder sourse
set(H264E_SRC
  src/H264CodeFrame.c
  src/H264EncApi.c
  src/H264Init.c
  src/H264Mad.c
  src/H264NalUnit.c
  src/H264PictureParameterSet.c
  src/H264PutBits.c
  src/H264RateControl.c
  src/H264Sei.c
  src/H264SequenceParameterSet.c
  src/H264Slice.c
  src/encasiccontroller.c
  src/encpreprocess.c
  src/h264e_api.c
  src/h264e_utils.c
  ) 

			
add_library(codec_h264e STATIC
  ${H264E_API}
  ${H264E_COMMON}
  ${H264E_HDR} 
  ${H264E_SRC} 
  )
target_link_libraries(codec_h264e mpp_base)
set_target_properties(codec_h264e PROPERTIES FOLDER "mpp/codec")
