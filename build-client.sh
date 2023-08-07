#!/usr/bin/env bash

set -x

IMAGE_TAG="${1:-armonik_sdk_client_test:0.1.0}"

# Get the absolute path of the current script and its directory
script_path="$(readlink -f "${BASH_SOURCE:-$0}")"
script_dir="$(dirname "$script_path")"

# Set the working directory to the parent directory of the script
working_dir="$script_dir"
cd "$working_dir"
working_dir="$(pwd -P)"
cd -

# Change to the working directory
cd "${working_dir}"

docker build -t "${IMAGE_TAG}" -f client.Dockerfile --progress plain .

# Compile the project source using the Docker image
docker run --rm -it -e Grpc__EndPoint="$1" "${IMAGE_TAG}"
