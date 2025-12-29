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
# Format: soc_name_pattern|hw_list|codec_enable_list
#   - soc_name_pattern: SOC name, comma-separated list, or regex pattern
#   - Use empty string to keep default (enable all codecs)
set(SOC_CONFIG_TABLE
    # RK3036
    "RK3036|VEPU1;VDPU341;VDPU1|AVSD;H263D;H264D;JPEGD;MPEG2D;MPEG4D;VP8D;AV1D;H264E"

    # RK3066/RK3188 (comma-separated list)
    "RK3066, RK3188|VEPU1;VDPU1|AVSD;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;AV1D;JPEGE"

    # RK3126/8 (regex pattern)
    "RK312[68]|VEPU1;VDPU341;VDPU1|AVSD;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;H264E;H265E;JPEGE"

    # RK3288
    "RK3288|VEPU1;VDPU341;VDPU1|AVSD;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;H264E;JPEGE"

    # RK3368
    "RK3368|VEPU1;VDPU341;VDPU1|AVSD;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;AV1D;H264E;JPEGE"

    # RK3399
    "RK3399|VEPU2;VDPU341;VDPU2|AVSD;AVS2D;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;H264E;JPEGE"

    # RK3228/9 (regex pattern)
    "RK322[89]|VEPU2;VDPU341;VDPU2|AVSD;AVS2D;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;H264E;H265E;JPEGE"

    # RK3328
    "RK3328|VEPU2;VEPU541;VDPU341;VDPU2|AVSD;AVS2D;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;H264E;H265E;JPEGE"

    # RV1126/RV1109 (comma-separated list)
    "RV1126, RV1109|VEPU2;VEPU541;VDPU341;VDPU2|AVSD;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP9D;AV1D;H264E;H265E;JPEGE"

    # RK3326/PX30 (comma-separated list)
    "RK3326, PX30|VEPU2;VDPU341;VDPU2|AVSD;AVS2D;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;H264E;H265E;JPEGE"

    # RK1808
    "RK1808|VEPU2;VDPU2|AVSD;AVS2D;H263D;H264D;JPEGD;MPEG2D;MPEG4D;VP8D;AV1D;H264E;JPEGE"

    # RK3566/7/8 series (regex pattern)
    "RK356[678]|VEPU541;VEPU2;VDPU34X;VDPU720;VDPU2|AVSD;AVS2D;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;H264E;H265E;JPEGE"

    # RK3588 series - flagship (all codecs enabled)
    "RK3588|VEPU580;VEPU2;VDPU2;VDPU34X;VDPU381;AV1D_VPU;VDPU720|"

    # RK3528 series
    "RK3528|VEPU540C;VDPU382;VDPU720;VDPU2|AVSD;AVS2D;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9E;AV1D;H264E;H265E;JPEGE"

    # RK3562
    "RK3562|VEPU540C;VDPU382;VDPU720|AVSD;AVS2D;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;JPEGE"

    # RK3576 series
    "RK3576|VEPU510;VEPU720;VDPU383;VDPU720|AVSD;AVS2D;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP8D;VP9D;AV1D;H264E;H265E;JPEGE"

    # RV1126B
    "RV1126B|VEPU511;VDPU384A|AVSD;H263D;H264D;H265D;JPEGD;MPEG2D;MPEG4D;VP9D;AV1D;H264E;H265E;JPEGE"

    # RK3538/9 (regex pattern)
    "RK353[89]|VDPU384B;VDPU730|AV1D;H264D;H265D;JPEGD"

    # RK3572
    "RK3572|VDPU384B;VDPU730;VEPU511;VEPU720|AV1D;H264D;H265D;JPEGD;VP9D;H264E;H265E;JPEGE"
)

# Helper macro to set all codec defaults
macro(set_all_codec_defaults value)
    foreach(codec ${MPP_CODEC_TYPES})
        set(ENABLE_${codec}_DEFAULT ${value})
    endforeach()
endmacro()

# Default to OFF (enable only what we need)
set_all_codec_defaults(OFF)

# Process SOC configuration
if (MPP_SOC)
    string(TOUPPER ${MPP_SOC} MPP_SOC_UPPER)
    message(STATUS "Configuring for SoC: ${MPP_SOC_UPPER}")

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

    # Helper function to auto-detect match type and check if SOC matches
    # Supports:
    #   - Exact match: "RK3588"
    #   - Multiple exact matches: "RK3066, RK3188"
    #   - Regex pattern: "RK356[678]" (contains regex special chars)
    function(soc_matches_pattern pattern soc_upper result_var)
        set(matched FALSE)

        # Check if pattern contains regex special characters
        if (pattern MATCHES "[\\[\\]\\*\\?\\+\\^\\$\\(\\)\\|]")
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
        # Split by "|", keeping empty fields
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
            if (NOT hw_list_str STREQUAL "")
                string(REPLACE ";" " " hw_list "${hw_list_str}")
                soc_enable_hw(${hw_list})
            endif()
            if (NOT codec_enable_str STREQUAL "")
                string(REPLACE ";" " " codec_enable_list "${codec_enable_str}")
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
        foreach(hw ${MPP_HW_TYPES})
            set(HAVE_${hw} ON)
            add_definitions(-DHW_${hw})
        endforeach()
        set_all_codec_defaults(ON)
    endif()

else()
    # Fallback: enable all if no SOC specified
    foreach(hw ${MPP_HW_TYPES})
        set(HAVE_${hw} ON)
        add_definitions(-DHW_${hw})
    endforeach()
    set_all_codec_defaults(ON)
endif()
