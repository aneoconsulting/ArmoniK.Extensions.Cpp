#!/usr/bin/env bash

set -x

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../../" )"
source "${script_path}"/../common.sh

IMAGE_TAG="${1:-"armoniksdktgz:${ARMONIK_SDK_VERSION_DEFAULT}"}"
API_VERSION="${2:-"${ARMONIK_API_VERSION_DEFAULT}"}"

docker build -t "${IMAGE_TAG}" -f ubi-tgz.Dockerfile --build-arg="API_VERSION=${API_VERSION}" --build-arg="VERSION=${ARMONIK_SDK_VERSION_DEFAULT}" "${working_dir}"

mkdir -p ${working_dir}/build

# Compile the project source using the Docker image
docker run --rm -v ".:/host" --entrypoint bash "${IMAGE_TAG}" -c "cp ./*.tar.gz /host/"
