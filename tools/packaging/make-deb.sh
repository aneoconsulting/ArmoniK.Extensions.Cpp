#!/usr/bin/env bash

set -x

. ../common.sh

IMAGE_TAG="${1:-"armoniksdkdeb:${ARMONIK_SDK_VERSION_DEFAULT}"}"
API_VERSION="${2:-"${ARMONIK_API_VERSION_DEFAULT}"}"

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../../" )"

docker build -t "${IMAGE_TAG}" -f deb.Dockerfile --build-arg="API_VERSION=${API_VERSION}" --build-arg="VERSION=${ARMONIK_SDK_VERSION_DEFAULT}" --progress=plain "${working_dir}"

mkdir -p ${working_dir}/build

# Compile the project source using the Docker image
docker run --rm -v ".:/host" --entrypoint bash "${IMAGE_TAG}" -c "cp ./*.deb /host/"
