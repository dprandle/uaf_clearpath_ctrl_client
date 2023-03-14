#!/bin/bash
urho_dir=../urho3d
ems_dir=../emsdk
while getopts "u:e:h" opt; do
    case $opt in
        u)
            urho_dir=$OPTARG
            ;;
        e)
            ems_dir=$OPTARG
            ;;
        h)
            echo "Specify the emsdk dir with -e path/to/emsdk and urho3d dir with -u path/to/urho3d"
            exit 1
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            exit 1
            ;;
    esac
done

if [ ! -d "${urho_dir}" ]; then
    echo "The specified directroy for Urho3D: ${urho_dir} does not exist."
    exit 1
fi

if [ ! -d "${ems_dir}" ]; then
    echo "The specified directroy for Emscripten: ${ems_dir} does not exist."
    exit 1
fi

ems_dir="$(cd "$ems_dir" && pwd -P)"

urho_samples="URHO3D_SAMPLES=OFF"
urho_opts="-D${urho_samples}"

ems_toolchain_file_opt="CMAKE_TOOLCHAIN_FILE=${ems_dir}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
ems_root_path_opt="EMSCRIPTEN_ROOT_PATH=${ems_dir}/upstream/emscripten"
ems_sysroot_opt="EMSCRIPTEN_SYSROOT=${ems_dir}/upstream/emscripten/system"
ems_mem_opt="EMSCRIPTEN_ALLOW_MEMORY_GROWTH=true"
ems_opts="-D${ems_toolchain_file_opt} -DWEB=true -D${ems_root_path_opt} -D${ems_sysroot_opt} -D${ems_mem_opt}"

# Create the commands so we can echo them easily before doing them
config_linux="cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=true ${urho_opts} -S ${urho_dir} -B ${urho_dir}/build/linux"
config_ems="cmake -DCMAKE_BUILD_TYPE=Release ${urho_opts} ${ems_opts} -S ${urho_dir} -B ${urho_dir}/build/emscripten"
build_linux="cmake --build ${urho_dir}/build/linux -- -j"
build_ems="cmake --build ${urho_dir}/build/emscripten -- -j"

echo "Configuring builds..."
echo ${config_linux}
${config_linux}
echo ${config_ems}
${config_ems}

mv ${urho_dir}/build/linux/compile_commands.json ${urho_dir}/build/compile_commands.json

echo "Building..."
echo ${build_linux}
${build_linux}
echo ${build_ems}
${build_ems}

