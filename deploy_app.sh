#!/bin/bash
deploy_local_cmd=
deploy_jackal_cmd=
deploy_husky_cmd=

while getopts ":l:j:h:" opt; do
    case $opt in
        l)
            server_dir=$OPTARG
            ;;
        j)
            jackal_hostname=$OPTARG
            ;;
        h)
            husky_hostname=$OPTARG
            ;;
        \?)
            echo "Invalid option: $OPTARG" >&2
            exit 1
            ;;
        :)
            if [[ $OPTARG == "l" ]]; then
                server_dir=../uaf_clearpath_ctrl_server
            elif [[ $OPTARG == "j" ]]; then
                jackal_hostname=cpr-uaf01
            elif [[ $OPTARG == "h" ]]; then
                husky_hostname=cpr-uaf02-husky
            else
                echo "Option -$OPTARG requires an argument." >&2
                exit 1
            fi
            ;;
    esac
done

build_glob="./build/emscripten/bin/uaf_*"

if [ -n "${server_dir}" ]; then
    cpy_cmd="cp ${build_glob} ${server_dir}/src/emscripten"
    echo ${cpy_cmd}
    ${cpy_cmd}
fi

if [ -n "${jackal_hostname}" ]; then
    cpy_cmd="scp ${build_glob} administrator@${jackal_hostname}:~/uaf_clearpath_ctrl_server/src/emscripten"
    echo ${cpy_cmd}
    ${cpy_cmd}
fi

if [ -n "${husky_hostname}" ]; then
    cpy_cmd="scp ${build_glob} administrator@${husky_hostname}:~/uaf_clearpath_ctrl_server/src/emscripten"
    echo ${cpy_cmd}
    ${cpy_cmd}
fi
