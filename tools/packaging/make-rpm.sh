#!/usr/bin/env bash

set -x

IMAGE_TAG="${1:-armoniksdkrpm:0.1.0}"

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../../" )"

docker build -t "${IMAGE_TAG}" -f ubi-rpm.Dockerfile --progress=plain "${working_dir}"

mkdir -p ${working_dir}/build

# Compile the project source using the Docker image
docker run --rm -v ".:/host" --entrypoint bash "${IMAGE_TAG}" -c "cp ./*.rpm /host/"
