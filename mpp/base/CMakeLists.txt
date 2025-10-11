# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp basic components
# ----------------------------------------------------------------------------
add_library(mpp_base OBJECT
    mpp_enc_refs.cpp
    mpp_enc_ref.cpp
    mpp_enc_cfg.c
    mpp_dec_cfg.c
    mpp_sys_cfg.c
    mpp_sys_cfg_st.c
    mpp_cluster.c
    mpp_buf_slot.cpp
    mpp_buffer_impl.c
    mpp_buffer.c
    mpp_packet.c
    mpp_frame.c
    mpp_task_impl.cpp
    mpp_task.cpp
    mpp_meta.c
    mpp_trie.c
    mpp_bitwrite.c
    mpp_bitread.c
    mpp_bitput.c
    mpp_cfg_io.c
    mpp_cfg.c
    mpp_2str.c
    mpp_dec_hdr_meta.c
    )

set_target_properties(mpp_base PROPERTIES FOLDER "mpp/base")

#target_link_libraries(mpp_base osal)

# unit test
add_subdirectory(test)
