# vim: syntax=cmake
include_directories(.)

# h265 encoder sourse
add_library(h265e_api OBJECT
    h265e_api.c
    h265e_dpb.c
    h265e_ps.c
    h265e_slice.c
    h265e_header_gen.c
    h265e_stream.c
    h265e_syntax.c
    h265e_enctropy.c
    )
