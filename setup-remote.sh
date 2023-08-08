#!/usr/bin/env bash

set -x

IMAGE_TAG="${1:-armonik_sdk_build_visual_studio:0.1.0}"

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path" )"

ARMONIK_API_DIR="$(realpath "${working_dir}/../ArmoniK.Api/packages/cpp/install")"

# Create an install directory and store its absolute path
install_dir="$(realpath "${working_dir}/install")"
mkdir -p "${install_dir}"

docker build -t "${IMAGE_TAG}" -f Dockerfile.vs "${working_dir}"

mkdir -p ${working_dir}/build

# Compile the project source using the Docker image
docker run -p 2223:22 -v "${working_dir}:/app/source" -v "${ARMONIK_API_DIR}:/armonik/api" -v "${install_dir}:/app/install" -v "${working_dir}/build:/app/build" --rm -it "${IMAGE_TAG}"