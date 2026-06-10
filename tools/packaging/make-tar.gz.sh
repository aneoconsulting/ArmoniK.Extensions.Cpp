#!/usr/bin/env bash

set -x

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../../" )"
source "${script_path}"/../common.sh

VERSION="${1:-"${ARMONIK_SDK_VERSION_DEFAULT}"}"
API_VERSION="${2:-"${ARMONIK_API_VERSION_DEFAULT}"}"
DOCKER_TAG="armoniksdktgz:${VERSION}"

docker build -t "${DOCKER_TAG}" \
    -f ubi8.Dockerfile \
    --build-arg="CPACK_GENERATOR=TGZ" \
    --build-arg="API_VERSION=${API_VERSION}" \
    --build-arg="WORKER_VERSION=${VERSION}" "${working_dir}"

mkdir -p ${working_dir}/build

# Compile the project source using the Docker image
docker run --rm -v ".:/host" --entrypoint bash "${DOCKER_TAG}" -c "cp ./*.tar.gz /host/"
