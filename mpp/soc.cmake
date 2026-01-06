# This file setup the default enable flag of all supported codecs
# The default value is ON for all codecs
# If MPP_SOC is defined, we will enable/disable codecs based on the SoC capability

# Define all supported codec types
set(MPP_CODEC_TYPES
    # decoders
    AVSD
    AVS2D
    H263D
    H264D
    H265D
    MPEG2D
    MPEG4D
    VP8D
    VP9D
    JPEGD
    AV1D

    # encoders
    H264E
    H265E
    JPEGE
    VP8E
)

# Define all supported hardware types
set(MPP_HW_TYPES
    # decoder
    VDPU1
    VDPU2
    VDPU341
    VDPU34X
    VDPU381
    VDPU382
    VDPU383
    VDPU384A
    VDPU384B
    VDPU720
    VDPU730
    AV1D_VPU

    # encoder
    VEPU1
    VEPU2
    VEPU541
    VEPU580
    VEPU540C
    VEPU510
    VEPU511
    VEPU720
)

# Define SOC configuration table
# Format: add_soc_config(soc_pattern hw_list decoders encoders)
#   - soc_pattern: SOC name, comma-separated list, or regex pattern
#                  Examples: "RK3588", "RK3066, RK3188", "RK356[678]"
#   - hw_list: Hardware modules (comma-separated)
#              Examples: "VDPU341,VDPU2,VEPU2"
#   - decoders: Supported decoder codecs (comma-separated, empty string if none)
#              Examples: "H264D,H265D,JPEGD" or ""
#   - encoders: Supported encoder codecs (comma-separated, empty string if none)
#              Examples: "H264E,JPEGE,VP8E" or ""
#
# Helper function to add a SOC configuration entry
function(add_soc_config soc_pattern hw_list decoders encoders)
    set(codec_list "")
    if (NOT decoders STREQUAL "")
        set(codec_list "${decoders}")
    endif()
    if (NOT encoders STREQUAL "")
        if (NOT codec_list STREQUAL "")
            set(codec_list "${codec_list},${encoders}")
        else()
            set(codec_list "${encoders}")
        endif()
    endif()
    set(entry "${soc_pattern}|${hw_list}|${codec_list}")
    list(APPEND SOC_CONFIG_TABLE "${entry}")
    set(SOC_CONFIG_TABLE "${SOC_CONFIG_TABLE}" PARENT_SCOPE)
endfunction()

# Add SOC configurations
add_soc_config("RK3036"
    "VDPU1,VDPU341,VDPU1"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD"
    "")

add_soc_config("RK3066, RK3188"
    "VDPU1,VDPU1,VEPU1"
    "H263D,H264D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD"
    "H264E,JPEGE,VP8E")

add_soc_config("RK3288"
    "VDPU341,VDPU1,VDPU1,VEPU1"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD"
    "H264E,JPEGE,VP8E")

add_soc_config("RK312[68]"
    "VDPU1,VDPU341,VDPU1,VEPU1"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD"
    "H264E,JPEGE,VP8E")

add_soc_config("RK3128H"
    "VDPU341,VDPU2,VEPU2"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD"
    "H264E,VP8E")

add_soc_config("RK3368"
    "VDPU341,VDPU1,VDPU1,VEPU1"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD"
    "H264E,JPEGE,VP8E")

add_soc_config("RK3399"
    "VDPU341,VDPU2,VEPU2"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AVSD"
    "H264E,JPEGE,VP8E")

add_soc_config("RK3328"
    "VDPU341,VDPU2,VEPU2,VEPU22"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AVSD"
    "H264E,JPEGE,VP8E,H265E")

add_soc_config("RK3228"
    "VDPU341,VDPU2,VEPU2"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AVSD"
    "H264E,VP8E")

add_soc_config("RK3228H"
    "VDPU341,VDPU2,VEPU2,VEPU22"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AVSD"
    "H264E,VP8E,H265E")

add_soc_config("RK3229"
    "VDPU341,VDPU2,VEPU2"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AVSD"
    "H264E,JPEGE,VP8E")

add_soc_config("RV1108"
    "VDPU2,VDPU341,VEPU2,VEPU541"
    "H264D,JPEGD"
    "H264E,JPEGE")

add_soc_config("RV1109, RV1126"
    "VDPU341,VDPU2,VEPU2,VEPU541"
    "H264D,H265D,JPEGD"
    "H264E,JPEGE,H265E")

add_soc_config("RK3326, PX30"
    "VDPU341,VDPU2,VEPU2"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD"
    "H264E,JPEGE,VP8E")

add_soc_config("RK1808"
    "VDPU2,VEPU2"
    "H263D,H264D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD"
    "H264E,JPEGE,VP8E")

add_soc_config("RK356[678]"
    "VDPU34X,VDPU720,VDPU2,VEPU541,VEPU2"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AVSD"
    "H264E,JPEGE,H265E")

add_soc_config("RK3588"
    "VDPU381,VDPU34X,VDPU720,VDPU2,AV1D_VPU,VEPU580,VEPU2"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AV1D,AVSD,AVS2D"
    "H264E,JPEGE,VP8E,H265E")

add_soc_config("RK3528"
    "VDPU382,VDPU720,VDPU2,VEPU540C"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,AVSD,AVS2D"
    "H264E,H265E,JPEGE")

add_soc_config("RK3528A"
    "VDPU382,VDPU720,VDPU2,VEPU540C"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AVSD,AVS2D"
    "H264E,H265E,JPEGE")

add_soc_config("RK3562"
    "VDPU382,VDPU720,VEPU540C"
    "H264D,H265D,JPEGD,VP9D"
    "H264E,JPEGE")

add_soc_config("RK3576"
    "VDPU383,VDPU720,VEPU510,VEPU720"
    "H264D,H265D,JPEGD,VP9D,AV1D,AVS2D"
    "H264E,H265E,JPEGE")

add_soc_config("RV1126B"
    "VDPU384A,VDPU720,VEPU511"
    "H264D,H265D,JPEGD"
    "H264E,H265E,JPEGE")

add_soc_config("RK353[89]"
    "VDPU384B,VDPU730,VDPU2"
    "H263D,H264D,H265D,JPEGD,MPEG2D,MPEG4D,VP8D,VP9D,AV1D,AVSD"
    "")

add_soc_config("RK3572"
    "VDPU384B,VDPU730,VEPU511,VEPU720"
    "H264D,H265D,JPEGD,VP9D,AV1D,AVS2D"
    "H264E,H265E,JPEGE")

# Helper macro to set all codec defaults
macro(set_all_codec_defaults value)
    foreach(codec ${MPP_CODEC_TYPES})
        set(ENABLE_${codec}_DEFAULT ${value})
    endforeach()
endmacro()

# Helper macro to enable HW
macro(soc_enable_hw)
    foreach(hw ${ARGN})
        set(HAVE_${hw} ON)
        add_definitions(-DHW_${hw})
    endforeach()
endmacro()

# Helper macro to enable codecs
macro(soc_enable_codec)
    foreach(codec ${ARGN})
        set(ENABLE_${codec}_DEFAULT ON)
    endforeach()
endmacro()

# Default to OFF (enable only what we need)
set_all_codec_defaults(OFF)

# Process SOC configuration
if (MPP_SOC)
    string(TOUPPER ${MPP_SOC} MPP_SOC_UPPER)
    message(STATUS "Configuring for SoC: ${MPP_SOC_UPPER}")


    # Helper function to auto-detect match type and check if SOC matches
    # Supports:
    #   - Exact match: "RK3588"
    #   - Multiple exact matches: "RK3066, RK3188"
    #   - Regex pattern: "RK356[678]" (contains regex special chars)
    function(soc_matches_pattern pattern soc_upper result_var)
        set(matched FALSE)

        # Check if pattern contains regex special characters
        if (pattern MATCHES "[]\\[^$|*+?()]")
            # Use regex matching
            if (soc_upper MATCHES ${pattern})
                set(matched TRUE)
            endif()
        elseif (pattern MATCHES ",")
            # Multiple exact matches separated by comma
            string(REPLACE "," ";" pattern_list ${pattern})
            foreach(p ${pattern_list})
                string(STRIP ${p} p_stripped)
                if (soc_upper STREQUAL p_stripped)
                    set(matched TRUE)
                    break()
                endif()
            endforeach()
        else()
            # Single exact match
            if (soc_upper STREQUAL pattern)
                set(matched TRUE)
            endif()
        endif()

        set(${result_var} ${matched} PARENT_SCOPE)
    endfunction()

    # Process SOC configuration
    set(SOC_FOUND FALSE)
    foreach(config_entry ${SOC_CONFIG_TABLE})

        # Split by "|" (pipe separates the three main fields)
        string(REPLACE "|" ";" config_list "${config_entry}")
        list(LENGTH config_list list_len)

        # Extract fields with safe defaults
        list(GET config_list 0 soc_pattern)
        set(hw_list_str "")
        set(codec_enable_str "")

        if (list_len GREATER 1)
            list(GET config_list 1 hw_list_str)
        endif()
        if (list_len GREATER 2)
            list(GET config_list 2 codec_enable_str)
        endif()

        # Auto-detect match type and check if SOC matches
        soc_matches_pattern("${soc_pattern}" "${MPP_SOC_UPPER}" matched)

        if (matched)
            message(STATUS "Applying SOC configuration for: ${soc_pattern}")
            message(STATUS "hw_list_str='${hw_list_str}', codec_enable_str='${codec_enable_str}'")
            if (NOT hw_list_str STREQUAL "")
                string(REPLACE "," ";" hw_list "${hw_list_str}")
                #message(STATUS "DEBUG: hw_list='${hw_list}'")
                soc_enable_hw(${hw_list})
            endif()
            if (NOT codec_enable_str STREQUAL "")
                string(REPLACE "," ";" codec_enable_list "${codec_enable_str}")
                #message(STATUS "DEBUG: codec_enable_list='${codec_enable_list}'")
                soc_enable_codec(${codec_enable_list})
            else()
                # Empty codec_enable_list means enable all codecs
                message(STATUS "  Enabling all codecs")
                set_all_codec_defaults(ON)
            endif()
            set(SOC_FOUND TRUE)
            break()
        endif()
    endforeach()

    if (NOT SOC_FOUND)
        message(WARNING "Unsupported SoC: ${MPP_SOC_UPPER}, falling back to generic configuration")
        unset(MPP_SOC)
        unset(MPP_SOC_UPPER)

        # Fallback: enable all hardware and codecs
        soc_enable_hw(${MPP_HW_TYPES})
        set_all_codec_defaults(ON)
    endif()
else()
    # Fallback: enable all if no SOC specified
    soc_enable_hw(${MPP_HW_TYPES})
    set_all_codec_defaults(ON)
endif()
