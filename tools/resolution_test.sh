#!/usr/bin/env bash
#########################################################################
# File Name: resolution_test.sh
# Author: LiHongjin
# mail: vic.hong@rock-chips.com
# Created Time: Mon 28 Apr 2025 08:39:38 AM CST
#########################################################################

# def test h265
cmd_spec="h265"
cmd_sv_file="false"
cmd_log="false"
cmd_debug="false"

rsl_list="
4x4
8x8
16x16
32x32
64x64
640x360
854x480
1280x720
1920x1080
2560x1440
3840x2160
16384x8192
16384x9384
16384x10384
16384x16384
65472x65472
65520x65520
"

spec_list="h264 h265 avs2 vp9 av1 jpeg"

type_h264="7"
type_h265="16777220"
type_avs2="16777223"
type_vp9="10"
type_av1="16777224"
type_jpeg="8"

enc_tool="mpi_enc_test"
dec_tool="mpi_dec_test"

test_spec_rsl()
{
    cur_sp="$1"

    echo
    echo "==> cur spec: ${cur_sp}"

    for cur_rsl in ${rsl_list}
    do
        enc_out_norm_path="/data/mpp_rsl_enc_norm_${cur_rsl}.${cur_sp}"
        enc_out_kmpp_path="/data/mpp_rsl_enc_kmpp_${cur_rsl}.${cur_sp}"
        dec_out_path="/data/mpp_rsl_dec_${cur_rsl}.${cur_sp}"
        frm_cnt=5

        enc_norm_res="--"
        enc_kmpp_res="--"
        dec_res="--"

        width=${cur_rsl%x*}
        height=${cur_rsl#*x}
        quiet_para=""
        [ "${cmd_log}" = "false" ] && quiet_para="> /dev/null 2>&1"
        [ "${width}" -gt 8192 ] && frm_cnt=2
        [ "${height}" -gt 8192 ] && frm_cnt=2
        [ "${cur_sp}" = "jpeg" ] && frm_cnt=1

        eval cur_type='$'type_${cur_sp}

        # enc normal
        [ -e "${enc_out_norm_path}" ] && rm ${enc_out_norm_path}
        cur_enc_cmd="${enc_tool} -w ${width} -h ${height} -n ${frm_cnt} -t ${cur_type} -o ${enc_out_norm_path} -rc 2 ${quiet_para}"
        [ ${cmd_debug} = "true" ] && echo "cur enc cmd: ${cur_enc_cmd}"
        eval ${cur_enc_cmd}
        [ "$?" -eq 0 ] && { enc_norm_res="pass"; } || { enc_norm_res="faile"; }
        if [ -e ${enc_out_norm_path} ]; then
            [ "`wc -c < ${enc_out_norm_path}`" -eq 0 ] && enc_norm_res="faile"
        fi

        # enc kmpp
        [ -e "${enc_out_kmpp_path}" ] && rm ${enc_out_kmpp_path}
        cur_enc_cmd="${enc_tool} -w ${width} -h ${height} -n ${frm_cnt} -t ${cur_type} -o ${enc_out_kmpp_path} -kmpp 1 ${quiet_para}"
        [ ${cmd_debug} = "true" ] && echo "cur enc cmd: ${cur_enc_cmd}"
        eval ${cur_enc_cmd}
        [ "$?" -eq 0 ] && { enc_kmpp_res="pass"; } || { enc_kmpp_res="faile"; }
        if [ -e ${enc_out_kmpp_path} ]; then
            [ "`wc -c < ${enc_out_kmpp_path}`" -eq 0 ] && enc_kmpp_res="faile"
        fi

        # dec
        if [ "${enc_norm_res}" = "pass" ]; then
            [ -e "${dec_out_path}" ] && rm ${dec_out_path}
            cur_dec_cmd="${dec_tool} -i ${enc_out_norm_path} -w ${width} -h ${height} -t ${cur_type} -o ${dec_out_path} ${quiet_para}"
            [ ${cmd_debug} = "true" ] && echo "cur dec cmd: ${cur_dec_cmd}"
            eval ${cur_dec_cmd}
            [ "$?" -eq 0 ] && { dec_res="pass"; } || { dec_res="faile"; }
            if [ -e ${dec_out_path} ]; then
                [ "`wc -c < ${dec_out_path}`" -eq 0 ] && dec_res="faile"
            fi
        elif [ ${enc_kmpp_res} = "pass" ];  then
            [ -e "${dec_out_path}" ] && rm ${dec_out_path}
            cur_dec_cmd="${dec_tool} -i ${enc_out_kmpp_path} -w ${width} -h ${height} -t ${cur_type} -o ${dec_out_path} ${quiet_para}"
            [ ${cmd_debug} = "true" ] && echo "cur dec cmd: ${cur_dec_cmd}"
            eval ${cur_dec_cmd}
            [ "$?" -eq 0 ] && { dec_res="pass"; } || { dec_res="faile"; }
            if [ -e ${dec_out_path} ]; then
                [ "`wc -c < ${dec_out_path}`" -eq 0 ] && dec_res="faile"
            fi
        fi

        printf "rsl: %-12s  enc_norm %-5s  enc_kmpp %-5s  dec %-5s\n" ${cur_rsl} ${enc_norm_res} ${enc_kmpp_res} ${dec_res}

        if [ "${cmd_sv_file}" = "false" ]; then
            [ -e ${enc_out_norm_path} ] && rm ${enc_out_norm_path}
            [ -e ${enc_out_kmpp_path} ] && rm ${enc_out_kmpp_path}
            [ -e ${dec_out_path} ] && rm ${dec_out_path}
        fi
    done
}

usage()
{
    spec_info=""
    for cur_sp in ${spec_list}
    do
        spec_info="${spec_info}|${cur_sp}"
    done
    spec_info="${spec_info}|all"
    echo "<exe> <-s|--spec> <${spec_info}> [-q]"
    echo "    -h|--help:  help info"
    echo "    -s|--spec:  spec, ${spec_info}"
    echo "    -sv|--save: save test file"
    echo "    -l|--log:   exec mpp demo with log"
    echo "    -d|--debug: dump debug info"
}

proc_paras()
{
    while [ $# -gt 0 ]; do
        key="$1"
        case ${key} in
            -h|--help) usage; exit 0; ;;
            -s|--spec) cmd_spec="$2"; shift; ;;
            -sv|--save) cmd_sv_file="true"; ;;
            -l|--log) cmd_log="true"; ;; -d|--debug) cmd_debug="true"; ;;
            *) usage; exit 1 ;;
        esac
        shift # move to next para
    done

    # check spec
    found=0
    for cur_sp in ${spec_list}; do [ "${cmd_spec}" = "${cur_sp}" ] && { found=1; break; } done
    [ "${cmd_spec}" = "all" ] && found=1
    [ ${found} -eq 0 ] && { echo "unknow spec: ${cmd_spec}"; exit 1; }

    # dump cmd info
    echo "cmd spec:    ${cmd_spec}"
    echo "cmd sv_file: ${cmd_sv_file}"
    echo "cmd log:     ${cmd_log}"
    echo "cmd debug:   ${cmd_debug}"
}


main()
{
    proc_paras $@
    if [ ${cmd_spec} = "all" ]
    then
        for cur_sp in ${spec_list}; do test_spec_rsl ${cur_sp}; done
    else
        test_spec_rsl ${cmd_spec}
    fi
}

main $@
