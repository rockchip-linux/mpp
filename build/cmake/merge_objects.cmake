# merge_objects.cmake
# Usage:
#   merge_objects(<out_obj>  <objLib1> [<objLib2> ...])
# Merge multiple OBJECT libraries → single large .o → immediately wrap into new OBJECT library (IMPORTED)
function(merge_objects out_obj)
    # 1. Sanity-check: make sure the caller actually gave us a non-empty output name
    if(out_obj STREQUAL "")
        return()
    endif()

    # 2. Collect all $<TARGET_OBJECTS:> and validate
    set(obj_inputs)
    set(deps)
    foreach(lib IN LISTS ARGN)
        # Skip if target doesn't exist
        if(NOT TARGET "${lib}")
            message(FATAL_ERROR "merge_objects: '${lib}' is not a target")
            continue()
        endif()

        get_target_property(_type "${lib}" TYPE)
        if(NOT _type STREQUAL "OBJECT_LIBRARY")
            message(FATAL_ERROR "merge_objects: '${lib}' is not an OBJECT library")
        endif()
        list(APPEND obj_inputs "$<TARGET_OBJECTS:${lib}>")
        list(APPEND deps "${lib}")
    endforeach()

    # If no valid targets to merge, skip merge
    if(NOT obj_inputs)
        message(STATUS "merge_objects: no valid targets to merge for '${out_obj}', skipping")
        return()
    endif()

    set(merged_o "${CMAKE_BINARY_DIR}/${out_obj}.o")
    set(rsp_file "${CMAKE_BINARY_DIR}/${out_obj}.rsp")

    # 3. Generate response file (one .o per line)
    file(GENERATE OUTPUT "${rsp_file}" CONTENT "$<JOIN:${obj_inputs},\n>")

    # 4. Merge command
    add_custom_command(
        OUTPUT  "${merged_o}"
        COMMAND ${CMAKE_LINKER} -r -o "${merged_o}" "@${rsp_file}"
        DEPENDS ${deps} "${rsp_file}"
        COMMENT "Merging OBJECT libs into ${out_obj}.o"
    )

    # 5. Immediately wrap into **IMPORTED OBJECT library**
    add_library("${out_obj}" OBJECT IMPORTED)
    set_target_properties("${out_obj}" PROPERTIES
                          IMPORTED_OBJECTS "${merged_o}")
    # Dependency chain: users must wait for large .o generation when linking
    add_dependencies("${out_obj}" "${out_obj}_gen")   # Pseudo target, ensure commands run first
    add_custom_target("${out_obj}_gen" DEPENDS "${merged_o}")
endfunction()