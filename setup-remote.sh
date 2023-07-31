#!/usr/bin/env bash

set -x

IMAGE_TAG="${1:-armonik_sdk_build_visual_studio:0.1.0}"

# Get the absolute path of the current script and its directory
script_path="$(readlink -f "${BASH_SOURCE:-$0}")"
script_dir="$(dirname "$script_path")"

# Set the working directory to the parent directory of the script
working_dir="$script_dir"
cd "$working_dir"
working_dir="$(pwd -P)"
cd -

# Mount the API install directory
cd "${working_dir}/../ArmoniK.Api/packages/cpp/install"
ARMONIK_API_DIR="$(pwd -P)"
cd -

# Create an install directory and store its absolute path
mkdir -p "${working_dir}/install"
cd "${working_dir}/install"
install_dir="$(pwd -P)"
cd -

# Change to the working directory
cd "${working_dir}"


docker build -t "${IMAGE_TAG}" -f Dockerfile.vs .

mkdir -p ${working_dir}/build

# Compile the project source using the Docker image
docker run -p 5001:22 -v "${working_dir}:/app/source" -v "${ARMONIK_API_DIR}:/armonik/api" -v "${working_dir}/install:/app/install" -v "${working_dir}/build:/app/build" --rm -it "${IMAGE_TAG}"
