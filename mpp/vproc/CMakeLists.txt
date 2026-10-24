# vim: syntax=cmake

if( HAVE_VPROC_IEP )
    add_subdirectory(iep)
endif()

if( HAVE_VPROC_IEP2 )
    add_subdirectory(iep2)
    install(FILES inc/iep_common.h inc/iep2_api.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rockchip")
endif()

if( HAVE_VPROC_VDPP )
    add_subdirectory(vdpp)
    install(FILES inc/hwpq_vdpp_proc_api.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rockchip")
endif()

# ----------------------------------------------------------------------------
# add mpp video process implement
# ----------------------------------------------------------------------------
add_library(mpp_vproc STATIC
    mpp_dec_vproc.c
    mpp_vproc_dev.c
    )

target_link_libraries(mpp_vproc
    ${VPROC_IEP}
    ${VPROC_IEP2}
    ${VPROC_VDPP}
    )
