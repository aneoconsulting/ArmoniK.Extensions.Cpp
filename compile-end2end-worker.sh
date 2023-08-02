#!/bin/sh
set -x

# Set the image tag for Docker
IMAGE_TAG="${1:-armonik_sdk_build:0.1.0}"

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path" )"

# Set the API install directory
ARMONIK_API_DIR="$(realpath "${working_dir}/../ArmoniK.Api/packages/cpp/install")"

# Check if the Docker image exists, and if not, build it
if [ -z "$(docker images -q "${IMAGE_TAG}" 2> /dev/null)" ]; then
  docker build -t "${IMAGE_TAG}" -f "${working_dir}/buildSDK.Dockerfile" "${working_dir}"
fi

mkdir -p "${working_dir}/build" "${working_dir}/install"

# Compile the project source using the Docker image
docker run -v "${working_dir}:/app/source" -v "${ARMONIK_API_DIR}:/armonik/api" -v "${working_dir}/install:/app/install" -v "${working_dir}/build:/app/build" --rm "${IMAGE_TAG}"
