# Run this from within a bash shell
MAKE_PROGRAM=`which make`

# delete list
FILES_TO_DELETE=(
    "CMakeCache.txt"
    "Makefile"
    "cmake_install.cmake"
    "compile_commands.json"
    "rockchip_mpp.pc"
    "rockchip_vpu.pc"
)

DIRS_TO_DELETE=(
    "CMakeFiles"
    "mpp"
    "osal"
    "test"
    "utils"
)

while [ $# -gt 0 ]; do
    case $1 in
        --help | -h)
            echo "Execute make-Makefiles.sh in *arm/* or *aarch64/* with some args."
            echo "  use --rebuild to rebuild after clean"
            echo "  use --clean to clean all build file"
            exit 1
            ;;
        --rebuild)
            ${MAKE_PROGRAM} clean
            if [ -f "CMakeCache.txt" ]; then
                rm CMakeCache.txt
            fi
            shift
            ;;
        --clean)
            for FILE_TO_DELETE in "${FILES_TO_DELETE[@]}"; do
                if [ -f ${FILE_TO_DELETE} ]; then
                    rm ${FILE_TO_DELETE}
                fi
            done
            for DIR_TO_DELETE in "${DIRS_TO_DELETE[@]}"; do
                if [ -d ${DIR_TO_DELETE} ]; then
                    rm -rf ${DIR_TO_DELETE}
                fi
            done
            exit 1
            ;;
    esac
    shift
done