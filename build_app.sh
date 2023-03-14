#!/bin/bash
urho_dir=../urho3d
ems_dir=
while getopts ":u:e:hw" opt; do
    case $opt in
        u)
            urho_dir=$OPTARG
            ;;
        e)
            ems_dir=$OPTARG
            ;;
        h)
            echo "Specify the emsdk dir with -e path/to/emsdk and urho3d dir with -u path/to/urho3d. No default for emsdk - without -e the linux build is used."
            exit 1
            ;;
        \?)
            echo "Invalid option: $OPTARG" >&2
            exit 1
            ;;
        :)
            if [[ $OPTARG == "e" ]]; then
               ems_dir=../emsdk
            else
                echo "Option -$OPTARG requires an argument." >&2
                exit 1
            fi
            ;;
    esac
done

if [ ! -d "${urho_dir}" ]; then
    echo "The specified directroy for Urho3D: ${urho_dir} does not exist."
    exit 1
fi

urho_opts="-DURHO3D_SRC=${urho_dir}"

if [ -d "${ems_dir}" ]; then
    ems_dir="$(cd "$ems_dir" && pwd -P)"
    echo "The specified directroy for Emscripten: ${ems_dir}"
    
    urho_opts="${urho_opts} -DURHO3D_HOME=${urho_dir}/build/emscripten"
    ems_toolchain_file_opt="CMAKE_TOOLCHAIN_FILE=${ems_dir}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
    ems_root_path_opt="EMSCRIPTEN_ROOT_PATH=${ems_dir}/upstream/emscripten"
    ems_sysroot_opt="EMSCRIPTEN_SYSROOT=${ems_dir}/upstream/emscripten/system"
    ems_mem_opt="EMSCRIPTEN_ALLOW_MEMORY_GROWTH=true"
    ems_opts="-D${ems_toolchain_file_opt} -DWEB=true -D${ems_root_path_opt} -D${ems_sysroot_opt} -D${ems_mem_opt}"

    config_ems="cmake -DCMAKE_BUILD_TYPE=Release ${urho_opts} ${ems_opts} -S ./ -B ./build/emscripten"
    build_ems="cmake --build ./build/emscripten -- -j"
    echo "Configuring..."
    echo ${config_ems}
    ${config_ems}
    echo "Building..."
    echo ${build_ems}
    ${build_ems}
else
    urho_opts="${urho_opts} -DURHO3D_HOME=${urho_dir}/build/linux"

    # Create the commands so we can echo them easily before doing them
    config_linux="cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=true ${urho_opts} -S ./ -B ./build/linux"
    build_linux="cmake --build ./build/linux -- -j"

    echo "Configuring builds..."
    echo ${config_linux}
    ${config_linux}
    mv ./build/linux/compile_commands.json ./build/compile_commands.json
    echo "Building..."
    echo ${build_linux}
    ${build_linux}
fi







