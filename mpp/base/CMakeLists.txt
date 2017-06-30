# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp basic components
# ----------------------------------------------------------------------------
add_library(mpp_base STATIC
    mpp_buf_slot.cpp
    mpp_buffer_impl.cpp
    mpp_buffer.cpp
    mpp_packet.cpp
    mpp_frame.cpp
    mpp_task_impl.cpp
    mpp_task.cpp
    mpp_meta.cpp
    mpp_bitread.c
    mpp_bitput.c
    )

set_target_properties(mpp_base PROPERTIES FOLDER "mpp/base")

target_link_libraries(mpp_base osal)

# unit test
add_subdirectory(test)
