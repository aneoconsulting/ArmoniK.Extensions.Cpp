#!/usr/bin/env bash

IMAGE_TAG="${1:-armonik_sdk_build_visual_studio:0.1.0}"

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../" )"

AK_API_DEFAULT="$(realpath "${working_dir}/../ArmoniK.Api/packages/cpp/install")"

if [[ -z "${ARMONIK_API_DIR}" ]]; then
    echo "Info: ARMONIK_API_DIR is not set"
    echo "      Using default $AK_API_DEFAULT"
    ARMONIK_API_DIR=AK_API_DEFAULT
fi

# Create an install directory and store its absolute path
install_dir="$(realpath "${working_dir}/install")"
mkdir -p "${install_dir}"

docker build -t "${IMAGE_TAG}" -f ${script_path=}/Dockerfile.vs "${working_dir}"

mkdir -p ${working_dir}/build

# Compile the project source using the Docker image
docker run -p 2223:22 \
    -v "${working_dir}:/app/source" \
    -v "${ARMONIK_API_DIR}:/armonik/api" \
    -v "${install_dir}:/app/install" \
    -v "${working_dir}/build:/app/build" \
    --rm -it "${IMAGE_TAG}"
